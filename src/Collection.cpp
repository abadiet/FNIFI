#include "fnifi/file/Collection.hpp"
#include "fnifi/utils/TempFile.hpp"
#include <csignal>
#include <sstream>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>

#define INFO_FILE "info.fnifi"
#define MAPPING_FILE "mapping.fnifi"
#define FILEPATHS_FILE "filepaths.fnifi"
#define FILEPATH_EMPTY_CHAR '?'


using namespace fnifi;
using namespace fnifi::file;

Collection::Collection(connection::IConnection* indexingConn,
                       const utils::SyncDirectory& storing)
: _indexingConn(indexingConn), _storing(storing),
    _storingPath(utils::Hash(getName())),
    _mapping(_storing.open(_storingPath / MAPPING_FILE)),
    _filepaths(_storing.open(_storingPath / FILEPATHS_FILE)),
    _info(_storing.open(_storingPath / INFO_FILE))
{
    DLOG("Collection", this, "Instanciation for IConnection " << indexingConn)

    /* setup _files and _availableIds */
    MapNode node;
    fileId_t id = 0;
    while (utils::Deserialize(_mapping, node)) {
        if (node.lenght > 0) {
            _files.insert({id, File(id, this)});
        } else {
            _availableIds.insert(id);
        }
        id++;
    }

    ILOG("Collection", this, "Found " << _files.size() << " files and "
         << _availableIds.size() << " available ids at initialisation")

    if (!_mapping.eof() && _mapping.fail()) {
        throw std::runtime_error("Error reading " +
                                 _mapping.getPath().string());
    }
    _mapping.clear();
}

Collection::~Collection() {
    if (_mapping.is_open()) {
        _mapping.close();
    }
    if (_filepaths.is_open()) {
        _filepaths.close();
    }
    if (_info.is_open()) {
        _info.close();
    }
}

void Collection::index(
    std::unordered_set<std::pair<const file::File*, fileId_t>>& removed,
    std::unordered_set<const file::File*>& added,
    std::unordered_set<file::File*>& modified)
{
    DLOG("Collection", this, "Indexation")

    _mapping.pull();
    _filepaths.pull();
    _info.pull();

    /* get current info, including last indexing time */
    Info info;
    _info.seekg(0, std::ios::end);
    if (_info.tellg() > 0) {
        /* the file is not empty */
        _info.seekg(0);
        utils::Deserialize(_info, info);
    }

    DLOG("Collection", this, "The most recent file already indexed has been "
         "created at " << S_TO_NS(info.lastIndexing.tv_sec) +
         info.lastIndexing.tv_nsec << "ns")

    /* unindex removed files and detect the ones that changed */
    for (auto it = _files.begin(); it != _files.end();) {
        const auto path = it->second.getPath();
        /* optimization: look for stat, if none, deduced the file does
         * not exists (avoid an additional call to IConnection::exists */
        const auto fileStat = it->second.getStats();
        if (fileStat.st_size == 0) {
            /* the file has been remove */
            const auto id = it->second.getId();

            ILOG("Collection", this, "File at \"" << path << "\" has been "
                 "removed")

            /* remove from _mapping */
            _mapping.seekg(id * sizeof(MapNode));
            MapNode node;
            utils::Deserialize(_mapping, node);
            std::string placeholderPath(node.lenght, FILEPATH_EMPTY_CHAR);
            node.lenght = 0;
            _mapping.seekp(id * sizeof(MapNode));
            utils::Serialize(_mapping, node);

            /* remove from _filepaths */
            _filepaths.seekp(std::streamoff(node.offset));
            _filepaths.write(placeholderPath.data(),
                             std::streamsize(placeholderPath.size()));

            _availableIds.insert(id);
            removed.insert({&it->second, id});
            it = _files.erase(it);
        } else {
            if (fileStat.st_mtimespec > info.lastIndexing) {

                ILOG("Collection", this, "File at \"" << path << "\" has been "
                     "modified")

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
            ILOG("Collection", this, "New file " << entry.path)
            /* TODO: check if the file is not already indexed (happens
             * when removed and then added */

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

                DLOG("Collection", this, "Recycle id " << id << " for the new "
                     "file " << entry.path)

                _mapping.seekp(id * sizeof(MapNode));
                _availableIds.erase(pos);
            } else {
                id = static_cast<fileId_t>(_files.size());
                _mapping.seekp(0, std::ios::end);
            }

            utils::Serialize(_mapping, node);

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
    _info.seekg(0);
    utils::Serialize(_info, info);

    DLOG("Collection", this, "The most recent file already indexed is now "
         "created at " << S_TO_NS(info.lastIndexing.tv_sec) +
         info.lastIndexing.tv_nsec << "ns")

    _mapping.push();
    _filepaths.push();
    _info.push();
}

void Collection::defragment() {
    DLOG("Collection", this, "Defragmentation")

    _mapping.pull();
    _filepaths.pull();

    /* find and remove unused chunks */
    std::vector<std::pair<offset_t, size_t>> chunks;
    {
        /* create a temporary file for the new content */
        utils::TempFile file;

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
                file.put(c);
            }
        }

        /* update _filepaths */
        _filepaths.take(file);
    }

    DLOG("Collection", this, "Found " << chunks.size() << " chunks to remove")

    /* adjust offsets */
    {
        _mapping.seekg(0);
        MapNode node;
        auto prevTellp = _mapping.tellp();
        while (utils::Deserialize(_mapping, node)) {
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
                utils::Serialize(_mapping, node);
            }
            prevTellp = _mapping.tellp();
        }
        if (!_mapping.eof() && _mapping.fail()) {
            std::ostringstream msg;
            msg << "Error while reading " << _mapping.getPath().string();
            ELOG("Collection", this, msg.str())
            throw std::runtime_error(msg.str());
        }
        _mapping.clear();
    }

    _mapping.push();
    _filepaths.push();
}

std::string Collection::getFilePath(fileId_t id) {
    /* get map's node */
    _mapping.seekg(id * sizeof(MapNode));
    MapNode node;
    utils::Deserialize(_mapping, node);

    if (node.lenght == 0) {
        std::ostringstream msg;
        msg << "Cannot get filepath for the file with id " << id << " as it no"
            " longer exists";
        ELOG("Collection", this, msg.str())
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

std::string Collection::getPreviewFilePath(fileId_t id) const {
    TODO
    UNUSED(id)
}
