#include "fnifi/file/Collection.hpp"
#include <sstream>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>

#define INFO_FILE "info.fnifi"
#define MAPPING_FILE "mapping.fnifi"
#define FILEPATHS_FILE "filepaths.fnifi"
#define TMP_SUFFIX ".tmp"
#define FILEPATH_EMPTY_CHAR '?'


using namespace fnifi;
using namespace fnifi::file;

Collection::Collection(connection::IConnection* indexingConn,
                       connection::IConnection* storingConn,
                       const std::filesystem::path& tmpPath)
: _indexingConn(indexingConn), _storingConn(storingConn),
    _tmpPath(tmpPath / Hash(getName()))
{
    /* create the folders if needed */
    std::filesystem::create_directories(_tmpPath);

    /* download files */
    pullStored();

    /* setup _files and _availableIds */
    {
        MapNode node;
        fileId_t id = 0;
        while (Deserialize(_mapping, node)) {
            if (node.lenght > 0) {
                _files.insert({id, File(id, this)});
            } else {
                _availableIds.insert(id);
            }
            id++;
        }
        DLOG("Collection " << this << " found " << _files.size()
             << " files and " << _availableIds.size()
             << " available ids at initialisation")
        if (!_mapping.eof() && _mapping.fail()) {
            throw std::runtime_error("Error reading " + (_tmpPath /
                                     MAPPING_FILE).string());
        }
        _mapping.clear();
    }
}

Collection::~Collection() {
    if (_mapping.is_open()) {
        _mapping.close();
    }
    if (_filepaths.is_open()) {
        _filepaths.close();
    }
}

void Collection::index(
    std::unordered_set<std::pair<const file::File*, fileId_t>>& removed,
    std::unordered_set<const file::File*>& added,
    std::unordered_set<const file::File*>& modified)
{
    DLOG("Collection " << this << " is indexing")

    pullStored();

    /* get current info, including last indexing time */
    Info info;
    {
        const auto filepath = _tmpPath / INFO_FILE;
        std::ifstream file(filepath, std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file " +
                                     filepath.string());
        }
        if (file.tellg() > 0) {
            /* the file is not empty */
            file.seekg(0);
            Deserialize(file, info);
        }
        file.close();
    }

    /* unindex removed files and detect the ones that changed */
    for (auto it = _files.begin(); it != _files.end();) {
        const auto path = it->second.getPath();
        /* TODO optimization: look for stat, if none, deduced the file does
         * not exists */
        if (!_indexingConn->exists(path)) {
            /* the file has been remove */
            const auto id = it->second.getId();

            DLOG("Collection " << this << " realized that file " << path
                 << " has been remove")

            /* remove from _mapping */
            _mapping.seekg(id * sizeof(MapNode));
            MapNode node;
            Deserialize(_mapping, node);
            std::string placeholderPath(node.lenght, FILEPATH_EMPTY_CHAR);
            node.lenght = 0;
            _mapping.seekp(id * sizeof(MapNode));
            Serialize(_mapping, node);

            /* remove from _filepaths */
            _filepaths.seekp(std::streamoff(node.offset));
            _filepaths.write(placeholderPath.data(),
                             std::streamsize(placeholderPath.size()));

            _availableIds.insert(id);
            removed.insert({&it->second, id});
            it = _files.erase(it);
        } else {
            if (it->second.getStats().st_mtimespec > info.lastIndexing) {
                /* the file has changed */
                modified.insert(&it->second);
            }

            it++;
        }
    }

    /* check for new files */
    struct timespec mostRecentTime = info.lastIndexing;
    for (const auto& entry : _indexingConn->iterate("")) {
        if (entry.ctime > info.lastIndexing) {
            DLOG("Collection " << this << " found new file " << entry.path)
            /* TODO: check if the file is not already indexed */

            /* add the filepath */
            const auto lenght = static_cast<lenght_t>(entry.path.size());
            _filepaths.seekp(0, std::ios::end);
            const auto offset = static_cast<offset_t>(_filepaths.tellp());
            _filepaths.write(&entry.path[0], lenght);

            /* add the mapping */
            const MapNode node = {offset, lenght};
            fileId_t id;
            if (!_availableIds.empty()) {
                /* use an unused id instead of a newer one */
                const auto pos = _availableIds.begin();
                id = *pos;
                _mapping.seekp(id * sizeof(MapNode));
                _availableIds.erase(pos);
            } else {
                id = static_cast<fileId_t>(_files.size());
                _mapping.seekp(0, std::ios::end);
            }

            Serialize(_mapping, node);

            /* add to _files */
            _files.insert({id, File(id, this)});

            added.insert(&_files.find(id)->second);

            if (entry.ctime > mostRecentTime) {
                mostRecentTime = entry.ctime;
            }
        }
    }
    info.lastIndexing = mostRecentTime;

    /* update info's file */
    {
        std::ofstream file(_tmpPath / INFO_FILE, std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file " + (_tmpPath /
                                     INFO_FILE).string());
        }
        Serialize(file, info);
        file.close();
    }

    _mapping.flush();
    _filepaths.flush();
    pushStored();
}

void Collection::defragment() {
    DLOG("Collection " << this << " is defragmenting")

    pullStored();

    /* find and remove unused chunks */
    std::vector<std::pair<offset_t, size_t>> chunks;
    {
        /* create a temporary file for the new content */
        const auto tmpPath = _tmpPath / FILEPATHS_FILE TMP_SUFFIX;
        std::ofstream tmpfile(tmpPath, std::ios::trunc);
        if (!tmpfile.is_open()) {
            std::ostringstream msg;
            msg << "Cannot open file " << tmpPath;
            throw std::runtime_error(msg.str());
        }

        /* find chunks and fill the temporary file */
        _filepaths.seekg(0);
        char c;
        bool inChunk = false;
        while (_filepaths.get(c)) {
            if (c == FILEPATH_EMPTY_CHAR) {
                if (inChunk) {
                    chunks.back().second++;
                } else {
                    chunks.push_back({
                        static_cast<offset_t>(_filepaths.tellg()) - 1, 1});
                    inChunk = true;
                }
            } else {
                inChunk = false;
                tmpfile.put(c);
            }
        }

        /* remove the former file and make the temporary one the new one */
        _filepaths.close();
        const auto path = _tmpPath / FILEPATHS_FILE;
        if (std::remove(path.c_str()) != 0) {
            std::ostringstream msg;
            msg << "Unable to remove file " << path;
            throw std::runtime_error(msg.str());
        }
        if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
            std::ostringstream msg;
            msg << "Unable to rename file " << tmpPath << " to " << path;
            throw std::runtime_error(msg.str());
        }
        _filepaths = std::fstream(path, std::ios::in | std::ios::out |
                                  std::ios::binary);
    }
    DLOG("Collection " << this << " found " << chunks.size()
         << " chunks to remove")

    /* adjust offsets */
    {
        _mapping.seekg(0);
        MapNode node;
        auto prevTellp = _mapping.tellp();
        while (Deserialize(_mapping, node)) {
            size_t sz = 0;
            auto it = chunks.begin();
            while (it != chunks.end() && it->first < node.offset) {
                /* this chunk is before the node, therefore the node's offset
                 * is impacted by the chunk removal */
                sz += it->second;
                it++;
            }
            if (sz > 0) {
                node.offset -= sz;
                _mapping.seekp(prevTellp);
                Serialize(_mapping, node);
            }
            prevTellp = _mapping.tellp();
        }
        if (!_mapping.eof() && _mapping.fail()) {
            throw std::runtime_error("Error reading " + (_tmpPath /
                                     MAPPING_FILE).string());
        }
        _mapping.clear();
    }

    pushStored();
}

std::string Collection::getFilePath(fileId_t id) {
    /* get map's node */
    _mapping.seekg(id * sizeof(MapNode));
    MapNode node;
    Deserialize(_mapping, node);

    if (node.lenght == 0) {
        std::ostringstream msg;
        msg << "File with id " << id << " no longer exists";
        throw std::runtime_error(msg.str());
    }

    /* get filepath */
    std::string path(node.lenght + 1, '\0');
    _filepaths.seekp(std::streamoff(node.offset));
    _filepaths.read(&path[0], node.lenght);

    return path;
}

struct stat Collection::getStats(fileId_t id) {
    const auto filepath = getFilePath(id);
    return _indexingConn->getStats(filepath);
}

fileBuf_t Collection::read(fileId_t id) {
    const auto filepath = getFilePath(id);
    return _indexingConn->read(filepath);
}

fileBuf_t Collection::preview(fileId_t id) {
    const auto filepath = getPreviewFilePath(id);
    return _indexingConn->read(filepath);
}

std::unordered_map<fileId_t, File>::const_iterator Collection::begin() const {
    return _files.begin();
}

std::unordered_map<fileId_t, File>::const_iterator Collection::end() const {
    return _files.end();
}

std::unordered_map<fileId_t, File>::iterator Collection::begin() {
    return _files.begin();
}

std::unordered_map<fileId_t, File>::iterator Collection::end() {
    return _files.end();
}

size_t Collection::size() const {
    return _files.size();
}

std::string Collection::getName() const {
    return _indexingConn->getName();
}

void Collection::pullStored() {
    /* TODO: MD5 check? */
    DLOG("Collection " << this << " is pulling stored files")
    if (_mapping.is_open()) {
        _mapping.close();
    }
    if (_filepaths.is_open()) {
        _filepaths.close();
    }
    {
        const auto path = _tmpPath / MAPPING_FILE;
        if (_storingConn->exists(MAPPING_FILE)) {
            _storingConn->download(MAPPING_FILE, path);
            _mapping = std::fstream(path, std::ios::in |
                                    std::ios::out | std::ios::binary);
        } else {
            _mapping = std::fstream(path, std::ios::in |
                                    std::ios::out | std::ios::binary |
                                    std::ios::trunc);
        }
        if (!_mapping.is_open()) {
            std::ostringstream msg;
            msg << "Cannot open file " << path;
            throw std::runtime_error(msg.str());
        }
    }
    {
        const auto path = _tmpPath / FILEPATHS_FILE;
        if (_storingConn->exists(FILEPATHS_FILE)) {
            _storingConn->download(FILEPATHS_FILE, path);
            _filepaths = std::fstream(path, std::ios::in |
                                    std::ios::out | std::ios::binary);
        } else {
            _filepaths = std::fstream(path, std::ios::in |
                                    std::ios::out | std::ios::binary |
                                    std::ios::trunc);
        }
        if (!_filepaths.is_open()) {
            std::ostringstream msg;
            msg << "Cannot open file " << path;
            throw std::runtime_error(msg.str());
        }
    }
    {
        const auto path = _tmpPath / INFO_FILE;
        if (_storingConn->exists(INFO_FILE)) {
            _storingConn->download(INFO_FILE, path);
        } else {
            std::fstream file(path, std::ios::in | std::ios::out |
                              std::ios::binary | std::ios::trunc);
            file.close();
        }
    }
}

void Collection::pushStored() {
    DLOG("Collection " << this << " is pushing stored files")
    {
        const auto path = _tmpPath / MAPPING_FILE;
        _storingConn->upload(path, MAPPING_FILE);
    }
    {
        const auto path = _tmpPath / FILEPATHS_FILE;
        _storingConn->upload(path, FILEPATHS_FILE);
    }
    {
        const auto path = _tmpPath / INFO_FILE;
        _storingConn->upload(path, INFO_FILE);
    }
}

std::string Collection::getPreviewFilePath(fileId_t id) const {
    TODO
    UNUSED(id);
}
