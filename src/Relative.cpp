#include "fnifi/connection/Relative.hpp"


using namespace fnifi;
using namespace fnifi::connection;

Relative::Relative(IConnection* conn, const std::filesystem::path& path)
: _conn(conn), _path(path)
{}

void Relative::connect() {
    _conn->connect();
}

void Relative::disconnect(bool agressive) {
    _conn->disconnect(agressive);
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

void Relative::download(const std::filesystem::path& from,
                        const std::filesystem::path& to) {
    _conn->download(_path / from, to);
}

void Relative::upload(const std::filesystem::path& from,
                      const std::filesystem::path& to) {
    _conn->upload(from, _path / to);
}

void Relative::remove(const std::filesystem::path& filepath) {
    _conn->remove(_path / filepath);
}

std::string Relative::getName() const {
    return "relative(" + _path.string() + ")-" + _conn->getName();
}
