#ifndef FNIFI_CONNECTION_RELATIVE_HPP
#define FNIFI_CONNECTION_RELATIVE_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils/utils.hpp"
#include <filesystem>


namespace fnifi {
namespace connection {

class Relative : virtual public IConnection {
public:
    Relative(IConnection* conn, const std::filesystem::path& path);
    void connect() override;
    void disconnect(bool force = false) override;
    DirectoryIterator iterate(const std::filesystem::path& path,
                              bool recursive = true, bool files = true,
                              bool folders = false) override;
    bool exists(const std::filesystem::path& filepath) override;
    struct stat getStats(const std::filesystem::path& filepath) override;
    fileBuf_t read(const std::filesystem::path& filepath) override;
    void write(const std::filesystem::path& filepath, const fileBuf_t& buffer)
        override;
    void download(const std::filesystem::path& from,
                  const std::filesystem::path& to) override;
    void upload(const std::filesystem::path& from,
                const std::filesystem::path& to) override;
    void remove(const std::filesystem::path& filepath) override;
    void createDirs(const std::filesystem::path& path) override;
    std::string getName() const override;

private:
    IConnection* _conn;
    const std::filesystem::path _path;
};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_RELATIVE_HPP */
