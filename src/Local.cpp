#include "fnifi/connection/Local.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>


using namespace fnifi;
using namespace fnifi::connection;

Local::Local() {}

void Local::connect() {}

void Local::disconnect(bool agressive) {
    UNUSED(agressive);
}

DirectoryIterator Local::iterate(const std::filesystem::path& path,
                                 bool recursive, bool files, bool folders)
{
    if (recursive) {
        return DirectoryIterator(
            std::filesystem::recursive_directory_iterator(path), files, folders
        );
    }
    return DirectoryIterator(std::filesystem::directory_iterator(path), files,
                             folders);
}

bool Local::exists(const std::filesystem::path& filepath) {
    return std::filesystem::exists(filepath);
}

struct stat Local::getStats(const std::filesystem::path& filepath) {
    struct stat fileStat;
    if (lstat(filepath.c_str(), &fileStat) != 0) {
        std::ostringstream msg;
        msg << "Failed to get stats for '" << filepath << "'";
        throw std::runtime_error(msg.str());
    }
    return fileStat;
}

fileBuf_t Local::read(const std::filesystem::path& filepath) {
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

void Local::write(const std::filesystem::path& filepath,
                  const fileBuf_t& buffer)
{
    std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::ostringstream msg;
        msg << "File '" << filepath << "' cannot be open";
        throw std::runtime_error(msg.str());
    }
    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));
}

void Local::download(const std::filesystem::path& from,
                     const std::filesystem::path& to)
{
    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive);
}

void Local::upload(const std::filesystem::path& from,
                   const std::filesystem::path& to)
{
    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive);
}

void Local::remove(const std::filesystem::path& filepath) {
    if (std::remove(filepath.c_str()) != 0) {
        std::ostringstream msg;
        msg << "File '" << filepath << "' cannot be remove";
        throw std::runtime_error(msg.str());
    }
}

std::string Local::getName() const {
    return "local";
}
