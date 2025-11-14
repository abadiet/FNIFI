#ifndef FNIFI_CONNECTION_LOCAL_HPP
#define FNIFI_CONNECTION_LOCAL_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils.hpp"
#include <filesystem>


namespace fnifi {
namespace connection {

class Local : virtual public IConnection {
public:
    Local();
    void connect() override;
    void disconnect() override;
    std::filesystem::recursive_directory_iterator iterate(const char* path)
        override;
    bool exists(const char* filepath) override;
    fileBuf_t read(const char* filepath) override;
    void write(const char* filepath, const fileBuf_t& buffer) override;
    void remove(const char* filepath) override;

private:

};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_LOCAL_HPP */
