#include "fnifi/connection/Local.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>


using namespace fnifi;
using namespace fnifi::connection;

Local::Local() {
    DLOG("Local", this, "Instanciation")
}

void Local::connect(unsigned int maxTry) {
    UNUSED(maxTry)
}

void Local::disconnect(bool force) {
    UNUSED(force)
}

DirectoryIterator Local::iterate(const std::filesystem::path& path,
                                 bool recursive, bool files, bool folders)
{
    DLOG("Local", this, "Iteration over path " << path)

    if (recursive) {
        return DirectoryIterator(
            std::filesystem::recursive_directory_iterator(path), files, folders
        );
    }
    return DirectoryIterator(std::filesystem::directory_iterator(path), files,
                             folders);
}

bool Local::exists(const std::filesystem::path& filepath) {
    DLOG("Local", this, "Existance check of " << filepath)

    return std::filesystem::exists(filepath);
}

struct stat Local::getStats(const std::filesystem::path& filepath) {
    DLOG("Local", this, "Get statistics for " << filepath)

    struct stat fileStat;
    if (lstat(filepath.c_str(), &fileStat) != 0) {
        fileStat.st_size = 0;
        WLOG("Local", this, "Failed to get stats for '" << filepath
             << ": will return the default instanciated stat with a null size")
    }
    return fileStat;
}

fileBuf_t Local::read(const std::filesystem::path& filepath) {
    DLOG("Local", this, "Read file " << filepath)

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        WLOG("Local", this, "Failed to open " << filepath
             << ": will return an empty buffer")
    }
    const fileBuf_t buffer((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    return buffer;
}

void Local::write(const std::filesystem::path& filepath,
                  const fileBuf_t& buffer)
{
    DLOG("Local", this, "Write to file " << filepath)

    std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        WLOG("SMB", this, "Failed to open " << filepath)
    }
    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));
}

bool Local::download(const std::filesystem::path& from,
                     const std::filesystem::path& to)
{
    DLOG("Local", this, "Download from " << from << " to " << to)

    if (!std::filesystem::exists(from)) {
        return false;
    }

    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive);

    return true;
}

bool Local::upload(const std::filesystem::path& from,
                   const std::filesystem::path& to)
{
    DLOG("Local", this, "Upload from " << from << " to " << to)

    std::filesystem::copy(from, to,
                          std::filesystem::copy_options::overwrite_existing |
                          std::filesystem::copy_options::recursive);

    return true;
}

void Local::remove(const std::filesystem::path& filepath) {
    DLOG("Local", this, "Remove file " << filepath)

    if (std::remove(filepath.c_str()) != 0) {
        WLOG("SMB", this, "Failed to remove " << filepath)
    }
}

void Local::createDirs(const std::filesystem::path& path) {
    DLOG("Local", this, "Create directories for path " << path)

    std::filesystem::create_directories(path);
}

std::string Local::getName() const {
    return "local";
}
