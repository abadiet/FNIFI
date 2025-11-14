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

Collection::Collection(connection::IConnection* conn, const char* indexingPath,
                       const char* storingPath)
: _conn(conn), _indexingPath(indexingPath), _storingPath(storingPath)
{
    {
        const auto path = _storingPath / MAPPING_FILE;
        _mapping = OpenOrCreateFile(path.c_str(), std::ios::in |
                                    std::ios::out | std::ios::binary);
    }
    {
        const auto path = _storingPath / FILEPATHS_FILE;
        _filepaths = OpenOrCreateFile(path.c_str(), std::ios::in |
                                      std::ios::out | std::ios::binary);
    }

    /* setup _files and _availableIds */
    {
        MapNode node;
        fileId_t id = 0;
        while (Deserialize(_mapping, node)) {
            if (node.lenght > 0) {
                _files.insert(File(id, this));
            } else {
                _availableIds.insert(id);
            }
            id++;
        }
        DLOG("Collection " << this << " found " << _files.size()
             << " files and " << _availableIds.size()
             << " available ids at initialisation")
        if (!_mapping.eof() && _mapping.fail()) {
            throw std::runtime_error("Error reading " + (_storingPath /
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

void Collection::index(std::unordered_set<fileId_t>& removed,
                       std::unordered_set<fileId_t>& added)
{
    /* unindex removed files */
    for (auto it = _files.begin(); it != _files.end();) {
        const auto path = _indexingPath / it->getPath();
        if (!_conn->exists(path.c_str())) {
            const auto id = it->getId();

            DLOG("Collection " << this << " realized that file " << id
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
            _filepaths.write(placeholderPath.c_str(),
                             std::streamsize(placeholderPath.size()));

            _availableIds.insert(id);
            removed.insert(id);
            it = _files.erase(it);
        } else {
            it++;
        }
    }

    /* get current info, including last indexing time */
    Info info;
    {
        const auto filepath = _storingPath / INFO_FILE;
        if (_conn->exists(filepath.c_str())) {
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
    }

    struct timespec mostRecentTime = info.lastIndexing;

    /* check for new files */
    for (const auto& entry : _conn->iterate(_indexingPath.c_str())) {
        if (entry.is_regular_file()) {
            struct stat fileStat;
            if (lstat(entry.path().string().c_str(), &fileStat) == 0) {
                const auto filetime = fileStat.st_ctimespec;
                if (filetime > info.lastIndexing) {
                    DLOG("Collection " << this << " found new file " << entry)
                    /* TODO: check if the file is not already indexed */

                    /* add the filepath */
                    const auto path =
                        std::filesystem::proximate(entry.path(), _indexingPath)
                        .string();
                    const auto lenght = static_cast<lenght_t>(path.size());
                    _filepaths.seekp(0, std::ios::end);
                    const auto offset = static_cast<offset_t>(
                        _filepaths.tellp());
                    _filepaths.write(&path[0], lenght);

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
                    _files.insert(File(id, this));

                    added.insert(id);

                    if (filetime > mostRecentTime) {
                        mostRecentTime = filetime;
                    }
                }
            } else {
                ELOG("Collection " << this << " failed to get the metadata of "
                     << entry << ". This file is ignored.")
            }
        }
    }

    info.lastIndexing = mostRecentTime;

    /* update info's file */
    {
        std::ofstream file(_storingPath / INFO_FILE, std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file " + (_storingPath /
                                     INFO_FILE).string());
        }
        Serialize(file, info);
        file.close();
    }

    _mapping.flush();
    _filepaths.flush();
}

void Collection::defragment() {
    /* find and remove unused chunks */
    std::vector<std::pair<offset_t, size_t>> chunks;
    {
        /* create a temporary file for the new content */
        const auto tmpPath = _storingPath / FILEPATHS_FILE TMP_SUFFIX;
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
        const auto path = _storingPath / FILEPATHS_FILE;
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
        _filepaths = OpenOrCreateFile(path.c_str(), std::ios::in |
                                      std::ios::out | std::ios::binary);
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
            throw std::runtime_error("Error reading " + (_storingPath /
                                     MAPPING_FILE).string());
        }
        _mapping.clear();
    }
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

fileBuf_t Collection::read(fileId_t id) {
    const auto filepath = _indexingPath / getFilePath(id);
    return _conn->read(filepath.c_str());
}

fileBuf_t Collection::preview(fileId_t id) {
    const auto filepath = getPreviewFilePath(id);
    return _conn->read(filepath.c_str());
}

std::unordered_set<File>::const_iterator Collection::begin() const {
    return _files.begin();
}

std::unordered_set<File>::const_iterator Collection::end() const {
    return _files.end();
}

size_t Collection::size() const {
    return _files.size();
}

std::string Collection::getPreviewFilePath(fileId_t id) const {
    TODO
    UNUSED(id);
}
