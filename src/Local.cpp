#include "fnifi/connection/Local.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>


using namespace fnifi;
using namespace fnifi::connection;

Local::Local() {}

void Local::connect() {}

void Local::disconnect() {}

DirectoryIterator Local::iterate(const char* path)
{
    return DirectoryIterator(
        std::filesystem::recursive_directory_iterator(path), "");
}

bool Local::exists(const char* filepath) {
    return std::filesystem::exists(filepath);
}

struct stat Local::getStats(const char* filepath) {
    struct stat fileStat;
    if (lstat(filepath, &fileStat) != 0) {
        std::ostringstream msg;
        msg << "Failed to get stats for '" << filepath << "'";
        throw std::runtime_error(msg.str());
    }
    return fileStat;
}

fileBuf_t Local::read(const char* filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::ostringstream msg;
        msg << "File '" << filepath << "' cannot be open";
        throw std::runtime_error(msg.str());
    }
    const fileBuf_t buffer((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    return buffer;
}

void Local::write(const char* filepath, const fileBuf_t& buffer) {
    std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::ostringstream msg;
        msg << "File '" << filepath << "' cannot be open";
        throw std::runtime_error(msg.str());
    }
    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));
}

void Local::download(const char* from, const char* to) {
    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive);
}

void Local::upload(const char* from, const char* to) {
    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive);
}

void Local::remove(const char* filepath) {
    if (std::remove(filepath) != 0) {
        std::ostringstream msg;
        msg << "File '" << filepath << "' cannot be remove";
        throw std::runtime_error(msg.str());
    }
}
