#ifndef FNIFI_CONNECTION_ICONNECTION_HPP
#define FNIFI_CONNECTION_ICONNECTION_HPP

#include "fnifi/utils/utils.hpp"
#include "fnifi/connection/DirectoryIterator.hpp"
#include <sys/stat.h>


namespace fnifi {
namespace connection {

class IConnection {
public:
    virtual ~IConnection();
    virtual void connect(unsigned int maxTry = 3) = 0;
    virtual void disconnect(bool force = false) = 0;
    virtual DirectoryIterator iterate(const std::filesystem::path& path,
                                      bool recursive = true, bool files = true,
                                      bool folders = false) = 0;
    virtual bool exists(const std::filesystem::path& filepath) = 0;
    virtual struct stat getStats(const std::filesystem::path& filepath) = 0;
    virtual fileBuf_t read(const std::filesystem::path& filepath) = 0;
    virtual void write(const std::filesystem::path& filepath,
                       const fileBuf_t& buffer) = 0;
    virtual bool download(const std::filesystem::path& from,
                          const std::filesystem::path& to) = 0;
    virtual bool upload(const std::filesystem::path& from,
                        const std::filesystem::path& to) = 0;
    virtual void remove(const std::filesystem::path& filepath) = 0;
    virtual void createDirs(const std::filesystem::path& path) = 0;
    virtual std::string getName() const = 0;
};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_ICONNECTION_HPP */
