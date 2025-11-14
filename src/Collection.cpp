#include "fnifi/file/Collection.hpp"
#include <filesystem>
#include <sstream>

#define OFFSETS_FILE "offsets.fnifi"
#define FILEPATHS_FILE "filepaths.fnifi"


using namespace fnifi;
using namespace fnifi::file;

Collection::Collection(connection::IConnection* conn, const char* indexingPath,
                       const char* storingPath)
: _conn(conn), _indexingPath(indexingPath), _storingPath(storingPath)
{
    {
        const std::filesystem::path path(_storingPath);
        _offsets = std::fstream(path / OFFSETS_FILE, std::ios::binary);
        if (!_offsets.is_open()) {
            throw std::runtime_error("Cannot open file '" + (path /
                                     OFFSETS_FILE).string() + "'");
        }
        _filepaths = std::fstream(path / FILEPATHS_FILE, std::ios::binary);
        if (!_filepaths.is_open()) {
            throw std::runtime_error("Cannot open file '" + (path /
                                     FILEPATHS_FILE).string() + "'");
        }
    }

    _offsets.seekg(0, std::ios::end);
    _nFiles = static_cast<fileId_t>(
        static_cast<size_t>(_offsets.tellg()) / sizeof(MapNode));
}

Collection::~Collection() {
    if (_offsets.is_open()) {
        _offsets.close();
    }
    if (_filepaths.is_open()) {
        _filepaths.close();
    }
}

void Collection::index() {
    /* check for new files */
}

fileId_t Collection::getFirstId() const {
    return 0;
}

fileId_t Collection::getLastId() const {
    return _nFiles;
}

std::string Collection::getFilePath(fileId_t id) {
    if (id >= _nFiles) {
        std::ostringstream msg;
        msg << "Id " << id << " is out of bounds [0, " << _nFiles << ")";
        throw std::runtime_error(msg.str());
    }

    /* get offset */
    _offsets.seekg(id * sizeof(MapNode));
    MapNode node;
    Deserialize(_offsets, node);

    if (node.lenght == 0) {
        std::ostringstream msg;
        msg << "File with id '" << id << "' no longer exists";
        throw std::runtime_error(msg.str());
    }

    /* get filepath */
    std::string path(node.lenght + 1, '\0');
    _filepaths.seekg(static_cast<std::streamoff>(node.offset));
    _filepaths.read(&path[0], node.lenght);

    return path;
}

fileBuf_t Collection::read(fileId_t id) {
    const auto filepath = getFilePath(id);
    return _conn->read(filepath.c_str());
}

fileBuf_t Collection::preview(fileId_t id) {
    const auto filepath = getPreviewFilePath(id);
    return _conn->read(filepath.c_str());
}

std::string Collection::getPreviewFilePath(fileId_t id) const {
    TODO
    UNUSED(id);
}
