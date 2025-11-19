#ifndef FNIFI_CONNECTION_LOCAL_HPP
#define FNIFI_CONNECTION_LOCAL_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils/utils.hpp"
#include "fnifi/connection/DirectoryIterator.hpp"
#include <sys/stat.h>


namespace fnifi {
namespace connection {

class Local : virtual public IConnection {
public:
    Local();
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
};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_LOCAL_HPP */
