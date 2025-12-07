#include "fnifi/file/Collection.hpp"
#include "fnifi/utils/TempFile.hpp"
#include <csignal>
#include <sstream>
#include <sys/stat.h>
#include <ctime>
#include <cstdio>
#include <opencv2/opencv.hpp>

#define INFO_FILE "info.fnifi"
#define MAPPING_FILE "mapping.fnifi"
#define FILEPATHS_FILE "filepaths.fnifi"
#define PREVIEW_DIRNAME "previews"
#define DEFAULT_PREVIEW_CHAR '?'
#define FILEPATH_EMPTY_CHAR '?'


using namespace fnifi;
using namespace fnifi::file;

Collection::Collection(connection::IConnection* indexingConn,
                       utils::SyncDirectory& storing)
: _indexingConn(indexingConn), _storing(storing),
    _storingPath(utils::Hash(getName())),
    _mapping(std::make_unique<utils::SyncDirectory::FileStream>
             (_storing, _storingPath / MAPPING_FILE)),
    _filepaths(std::make_unique<utils::SyncDirectory::FileStream>
               (_storing, _storingPath / FILEPATHS_FILE)),
    _info(std::make_unique<utils::SyncDirectory::FileStream>
          (_storing, _storingPath / INFO_FILE)), _previewsMkdir(true)
{
    DLOG("Collection", this, "Instanciation for IConnection " << indexingConn
         << " and SyncDirectory " << &storing)
}

Collection::Collection(Collection&& other) noexcept
    : _files(),
    _indexingConn(other._indexingConn),
    _storing(other._storing),
    _storingPath(std::move(other._storingPath)),
    _mapping(std::make_unique<utils::SyncDirectory::FileStream>
             (_storing, _storingPath / MAPPING_FILE)),
    _filepaths(std::make_unique<utils::SyncDirectory::FileStream>
               (_storing, _storingPath / FILEPATHS_FILE)),
    _info(std::make_unique<utils::SyncDirectory::FileStream>
          (_storing, _storingPath / INFO_FILE)),
    _availableIds(),
    _previewsMkdir(other._previewsMkdir)
{}

Collection::~Collection() {
    if (_mapping->is_open()) {
        _mapping->close();
    }
    if (_filepaths->is_open()) {
        _filepaths->close();
    }
    if (_info->is_open()) {
        _info->close();
    }
}

void Collection::index(
    std::unordered_set<std::pair<const file::File*, fileId_t>>& removed,
    std::unordered_set<const file::File*>& added,
    std::unordered_set<file::File*>& modified)
{
    DLOG("Collection", this, "Indexation")

    _mapping->pull();
    _filepaths->pull();
    _info->pull();

    /* retrieve files */
    /* TODO: update files thanks to _mapping everytime, not if _files is empty
     */
    if (_files.empty()) {
        MapNode node;
        fileId_t id = 0;
        while (utils::Deserialize(*_mapping, node)) {
            if (node.lenght > 0) {
                _files.insert({id, File(id, this)});
            } else {
                _availableIds.insert(id);
            }
            id++;
        }
        ILOG("Collection", this, "Found " << _files.size() << " files and "
             << _availableIds.size() << " available ids")
        if (!_mapping->eof() && _mapping->fail()) {
            throw std::runtime_error("Error reading " +
                                     _mapping->getPath().string());
        }
        _mapping->clear();
    }

    /* get current info, including last indexing time */
    Info info;
    _info->seekg(0, std::ios::end);
    if (_info->tellg() > 0) {
        /* the file is not empty */
        _info->seekg(0);
        utils::Deserialize(*_info, info);
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
            _mapping->seekg(id * sizeof(MapNode));
            MapNode node;
            utils::Deserialize(*_mapping, node);
            std::string placeholderPath(node.lenght, FILEPATH_EMPTY_CHAR);
            node.lenght = 0;
            _mapping->seekp(id * sizeof(MapNode));
            utils::Serialize(*_mapping, node);

            /* remove from _filepaths */
            _filepaths->seekp(std::streamoff(node.offset));
            _filepaths->write(placeholderPath.data(),
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
            _filepaths->seekp(0, std::ios::end);
            const auto offset = static_cast<offset_t>(_filepaths->tellp());
            _filepaths->write(&entry.path[0], lenght);

            /* add the mapping */
            const MapNode node = {offset, lenght};
            fileId_t id;
            if (!_availableIds.empty()) {
                /* use an unused id instead of a newer one */
                const auto pos = _availableIds.begin();
                id = *pos;

                DLOG("Collection", this, "Recycle id " << id << " for the new "
                     "file " << entry.path)

                _mapping->seekp(id * sizeof(MapNode));
                _availableIds.erase(pos);
            } else {
                id = static_cast<fileId_t>(_files.size());
                _mapping->seekp(0, std::ios::end);
            }

            utils::Serialize(*_mapping, node);

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
    _info->seekg(0);
    utils::Serialize(*_info, info);

    DLOG("Collection", this, "The most recent file already indexed is now "
         "created at " << S_TO_NS(info.lastIndexing.tv_sec) +
         info.lastIndexing.tv_nsec << "ns")

    _mapping->push();
    _filepaths->push();
    _info->push();
}

void Collection::defragment() {
    DLOG("Collection", this, "Defragmentation")

    _mapping->pull();
    _filepaths->pull();

    /* find and remove unused chunks */
    std::vector<std::pair<offset_t, size_t>> chunks;
    {
        /* create a temporary file for the new content */
        utils::TempFile file;

        /* find chunks and fill the temporary file */
        _filepaths->seekg(0);
        char c;
        bool inChunk = false;
        while (_filepaths->get(c)) {
            if (c == FILEPATH_EMPTY_CHAR) {
                if (inChunk) {
                    chunks.back().second++;
                } else {
                    chunks.push_back({
                        static_cast<offset_t>(_filepaths->tellg()) - 1, 1});
                    inChunk = true;
                }
            } else {
                inChunk = false;
                file.put(c);
            }
        }

        /* update _filepaths */
        _filepaths->take(file);
    }

    DLOG("Collection", this, "Found " << chunks.size() << " chunks to remove")

    /* adjust offsets */
    {
        _mapping->seekg(0);
        MapNode node;
        auto prevTellp = _mapping->tellp();
        while (utils::Deserialize(*_mapping, node)) {
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
                _mapping->seekp(prevTellp);
                utils::Serialize(*_mapping, node);
            }
            prevTellp = _mapping->tellp();
        }
        if (!_mapping->eof() && _mapping->fail()) {
            std::ostringstream msg;
            msg << "Error while reading " << _mapping->getPath().string();
            ELOG("Collection", this, msg.str())
            throw std::runtime_error(msg.str());
        }
        _mapping->clear();
    }

    _mapping->push();
    _filepaths->push();
}

std::string Collection::getFilePath(fileId_t id) {
    /* get map's node */
    _mapping->seekg(id * sizeof(MapNode));
    MapNode node;
    utils::Deserialize(*_mapping, node);

    if (node.lenght == 0) {
        std::ostringstream msg;
        msg << "Cannot get filepath for the file with id " << id << " as it no"
            " longer exists";
        ELOG("Collection", this, msg.str())
        throw std::runtime_error(msg.str());
    }

    /* get filepath */
    std::string path(node.lenght + 1, '\0');
    _filepaths->seekp(std::streamoff(node.offset));
    _filepaths->read(&path[0], node.lenght);

    return path;
}

struct stat Collection::getStats(fileId_t id) {
    const auto filepath = getFilePath(id);
    return _indexingConn->getStats(filepath);
}

Kind Collection::getKind(fileId_t id) {
    const auto content = read(id);
    return GetKind(content);
}

fileBuf_t Collection::read(fileId_t id) {
    const auto filepath = getFilePath(id);
    return _indexingConn->read(filepath);
}

fileBuf_t Collection::preview(fileId_t id) {
    const auto filepath = _storingPath / PREVIEW_DIRNAME / std::to_string(id);

    auto file = _storing.open(filepath, true, _previewsMkdir);
    if (_previewsMkdir) {
        /* Optimization: assuming the dirs will not be removed during the
         * lifetime of the program. Avoid a lot of unecessary calls to mkdir */
        _previewsMkdir = false;
    }

    const auto sz = file.tellg();
    if (sz == 0) {
        /* the file did not exists and has to be created */
        const auto original = read(id);

        const auto type = GetKind(original);
        switch (type) {
            case JPEG:
            case PNG:
                {
                    const auto img = cv::imdecode(original, cv::IMREAD_COLOR);
                    if (img.empty()) {
                        /* failed to decode image data */
                        break;
                    }
                    cv::Mat res;
                    cv::resize(img, res, cv::Size(128, 128));
                    fileBuf_t buffer;
                    cv::imencode(".jpg", res, buffer,
                                 {cv::IMWRITE_JPEG_QUALITY, 70});
                    file.write(reinterpret_cast<const char*>(buffer.data()),
                               std::streamsize(buffer.size()));
                    file.push();
                    file.close();
                    return buffer;
                }
            case UNKNOWN:
                /* cannot handle this file type: set the default preview */
                break;
        }

        file.put(DEFAULT_PREVIEW_CHAR);
        file.push();
        file.close();
        return {};
    }

    if (sz == 1) {
        /* cannot handle this file type */
        return {};
    }

    const fileBuf_t buf((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return buf;
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

Kind Collection::GetKind(const fileBuf_t& buf) {
    if (buf.size() > 3 && buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF) {
        return Kind::JPEG;
    }
    if (buf.size() > 4 && buf[0] == 0x89 && buf[1] == 0x50 && buf[2] == 0x4E &&
        buf[3] == 0x47)
    {
        return Kind::PNG;
    }
    return Kind::UNKNOWN;
}
