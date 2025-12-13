#include "fnifi/connection/Relative.hpp"


using namespace fnifi;
using namespace fnifi::connection;

Relative::Relative(IConnection* conn, const std::filesystem::path& path)
: _conn(conn), _path(path)
{
    DLOG("Relative", this, "Instanciation for IConnection " << conn << " and "
         "path " << path)
}

void Relative::connect(unsigned int maxTry) {
    _conn->connect(maxTry);
}

void Relative::disconnect(bool force) {
    _conn->disconnect(force);
}

DirectoryIterator Relative::iterate(const std::filesystem::path& path,
                                    bool recursive, bool files, bool folders)
{
    const auto dirit = _conn->iterate(_path / path, recursive, files,
                                      folders);
    return DirectoryIterator(dirit, _path);
}

bool Relative::exists(const std::filesystem::path& filepath) {
    return _conn->exists(_path / filepath);
}

struct stat Relative::getStats(const std::filesystem::path& filepath) {
    return _conn->getStats(_path / filepath);
}

fileBuf_t Relative::read(const std::filesystem::path& filepath) {
    return _conn->read(_path / filepath);
}

void Relative::write(const std::filesystem::path& filepath,
                     const fileBuf_t& buffer) {
    return _conn->write(_path / filepath, buffer);
}

bool Relative::download(const std::filesystem::path& from,
                        const std::filesystem::path& to) {
    return _conn->download(_path / from, to);
}

bool Relative::upload(const std::filesystem::path& from,
                      const std::filesystem::path& to) {
    return _conn->upload(from, _path / to);
}

void Relative::remove(const std::filesystem::path& filepath) {
    _conn->remove(_path / filepath);
}

void Relative::createDirs(const std::filesystem::path& path) {
    _conn->createDirs(_path / path);
}

std::string Relative::getName() const {
    return "relative(" + _path.string() + ")-" + _conn->getName();
}
