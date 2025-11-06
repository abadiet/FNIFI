#include "fnifi/Collection.hpp"


using namespace fnifi;

Collection::Collection(connection::IConnection* conn, const char* indexingPath,
                       const char* storingPath)
: _conn(conn), _indexingPath(indexingPath), _storingPath(storingPath)
{

}

fileId_t Collection::getFirstId() const {
    return 0;
}

fileId_t Collection::getLastId() const {
    return _nFiles;
}

std::string Collection::getFilePath(fileId_t id) const {
    TODO
    UNUSED(id);
}

fileBuf_t Collection::read(fileId_t id) {
    const auto filepath = getFilePath(id);
    return _conn->read(filepath.c_str());
}

fileBuf_t Collection::preview(fileId_t id) {
    const auto filepath = getPreviewFilePath(id);
    return _conn->read(filepath.c_str());
}

void Collection::write(fileId_t id, fileBuf_t buffer) {
    const auto filepath = getFilePath(id);
    _conn->write(filepath.c_str(), buffer);
}

void Collection::remove(fileId_t id) {
    const auto filepath = getFilePath(id);
    _conn->remove(filepath.c_str());
}

std::string Collection::getPreviewFilePath(fileId_t id) const {
    TODO
    UNUSED(id);
}
