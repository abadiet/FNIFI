#include "fnifi/connection/Local.hpp"
#include <fstream>
#include <sstream>


using namespace fnifi;
using namespace fnifi::connection;

Local::Local() {}

void Local::connect() {}

void Local::disconnect() {}

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
    file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
}

void Local::remove(const char* filepath) {
    if (std::remove(filepath) != 0) {
        std::ostringstream msg;
        msg << "File '" << filepath << "' cannot be remove";
        throw std::runtime_error(msg.str());
    }
}
