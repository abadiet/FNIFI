#include "fnifi/utils/TempFile.hpp"
#include "fnifi/utils/utils.hpp"
#include <ctime>
#include <cstdlib>
#include <sstream>


using namespace fnifi;
using namespace utils;

const char TempFile::_charset[63] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

char TempFile::RandomChar() {
    return _charset[static_cast<size_t>(std::rand()) % (sizeof(_charset) - 1)];
}

std::string TempFile::RandomString(unsigned char size) {
    std::string str(size, 0);
    std::generate_n(str.begin(), size, RandomChar);
    return str;
}

std::filesystem::path TempFile::RandomFilepath(unsigned char size) {
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        RandomString(size);
    while (std::filesystem::exists(path)) {
        path = std::filesystem::temp_directory_path() /
            RandomString(size);
    }
    return path;
}

TempFile::TempFile()
: _path(RandomFilepath(8))
{
    DLOG("TempFile", this, "Instanciation with path " << _path)

    open(_path, std::ios::in | std::ios::out | std::ios::binary |
         std::ios::trunc);

    if (!is_open()) {
        std::ostringstream msg;
        msg << "Could not create file " << _path;
        ELOG("TempFile", this, msg.str())
        throw std::runtime_error(msg.str());
    }
}

TempFile::~TempFile() {
    if (is_open()) {
        close();
    }
    if (std::filesystem::exists(_path)) {
        std::filesystem::remove(_path);
    }
}

std::filesystem::path TempFile::getPath() const {
    return _path;
}
