#include "fnifi/File.hpp"


using namespace fnifi;

File::File(fileId_t id, Collection* coll)
: _id(id), _coll(coll)
{

}

std::string File::getPath() const {
    return _coll->getFilePath(_id);
}

std::string File::getMetadata(const char* key) {
    const auto buf = _coll->read(_id);
    TODO
    UNUSED(key);
}

fileBuf_t File::preview() const {
    return _coll->preview(_id);
}

fileBuf_t File::read() const {
    return _coll->read(_id);
}

void File::write(const fileBuf_t& buf) {
    _coll->write(_id, buf);
}

void File::remove() const {
    _coll->remove(_id);
}
