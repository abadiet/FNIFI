#ifndef FNIFI_CONNECTION_RELATIVE_HPP
#define FNIFI_CONNECTION_RELATIVE_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils.hpp"
#include <filesystem>


namespace fnifi {
namespace connection {

class Relative : virtual public IConnection {
public:
    Relative(IConnection* conn, const char* path);
    void connect() override;
    void disconnect() override;
    DirectoryIterator iterate(const char* path) override;
    bool exists(const char* filepath) override;
    fileBuf_t read(const char* filepath) override;
    void write(const char* filepath, const fileBuf_t& buffer) override;
    void download(const char* from, const char* to) override;
    void upload(const char* from, const char* to) override;
    void remove(const char* filepath) override;

private:
    IConnection* _conn;
    const std::filesystem::path _path;
};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_RELATIVE_HPP */
