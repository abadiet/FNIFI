#ifndef FNIFI_CONNECTION_ICONNECTION_HPP
#define FNIFI_CONNECTION_ICONNECTION_HPP

#include "fnifi/utils.hpp"
#include <filesystem>


namespace fnifi {
namespace connection {

class IConnection {
public:
    virtual ~IConnection();
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual std::filesystem::recursive_directory_iterator iterate(
        const char* path) = 0;
    virtual bool exists(const char* filepath) = 0;
    virtual fileBuf_t read(const char* filepath) = 0;
    virtual void write(const char* filepath, const fileBuf_t& buffer) = 0;
    virtual void remove(const char* filepath) = 0;
};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_ICONNECTION_HPP */
