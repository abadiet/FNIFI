#include "fnifi/connection/Relative.hpp"


using namespace fnifi;
using namespace fnifi::connection;

Relative::Relative(IConnection* conn, const char* path)
: _conn(conn), _path(path)
{}

void Relative::connect() {
    _conn->connect();
}

void Relative::disconnect(bool agressive) {
    _conn->disconnect(agressive);
}

DirectoryIterator Relative::iterate(const char* path) {
    const auto abspath = _path / path;
    const auto dirit = _conn->iterate(abspath.c_str());
    return DirectoryIterator(dirit, _path.c_str());
}

bool Relative::exists(const char* filepath) {
    const auto abspath = _path / filepath;
    return _conn->exists(abspath.c_str());
}

struct stat Relative::getStats(const char* filepath) {
    const auto abspath = _path / filepath;
    return _conn->getStats(abspath.c_str());
}

fileBuf_t Relative::read(const char* filepath) {
    const auto abspath = _path / filepath;
    return _conn->read(abspath.c_str());
}

void Relative::write(const char* filepath, const fileBuf_t& buffer) {
    const auto abspath = _path / filepath;
    return _conn->write(abspath.c_str(), buffer);
}

void Relative::download(const char* from, const char* to) {
    const auto abspath = _path / from;
    _conn->download(abspath.c_str(), to);
}

void Relative::upload(const char* from, const char* to) {
    const auto abspath = _path / to;
    _conn->upload(from, abspath.c_str());
}

void Relative::remove(const char* filepath) {
    const auto abspath = _path / filepath;
    _conn->remove(abspath.c_str());
}
