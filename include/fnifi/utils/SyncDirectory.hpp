#ifndef FNIFI_UTILS_SYNCDIRECTORY_HPP
#define FNIFI_UTILS_SYNCDIRECTORY_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils/utils.hpp"
#include "fnifi/utils/TempFile.hpp"
#include <fstream>


namespace fnifi {
namespace utils {

class SyncDirectory {
public:
    class FileStream : public std::fstream {
    public:
        FileStream(const SyncDirectory& sync,
                   const std::filesystem::path& filepath, bool ate = false);
        virtual ~FileStream() override;
        bool pull();
        void push();
        void disableSync(bool pull = true);
        void enableSync(bool push = true);
        void take(TempFile& file);
        std::filesystem::path getPath(bool relative = false) const;

    private:
        FileStream(const std::filesystem::path& abspath,
                   const std::filesystem::path& relapath, bool ate,
                   const SyncDirectory& sync);
        void setup(bool ate);

        const SyncDirectory& _sync;
        const std::filesystem::path _abspath;
        const std::filesystem::path _relapath;
        bool _syncDisabled;

        friend SyncDirectory;
    };

    SyncDirectory(connection::IConnection* conn, const std::string& path);
    FileStream open(const std::filesystem::path& filepath, bool ate = false,
                    bool mkdir = true) const;
    bool exists(const std::filesystem::path& filepath) const;
    std::filesystem::path absolute(const std::filesystem::path& filepath)
        const;
    void remove(const std::filesystem::path& filepath) const;
    void createDirs(const std::filesystem::path& dirpath) const;

private:
    std::filesystem::path setupFileStream(
        const std::filesystem::path& filepath, bool mkdir = true) const;
    bool pull(const std::filesystem::path& abspath,
              const std::filesystem::path& relapath) const;
    void push(const std::filesystem::path& relapath, const fileBuf_t& buf)
        const;

    connection::IConnection* _conn;
    const std::filesystem::path _path;
};

}  /* namesapce connection */
}  /* namesapce fnifi */

#endif  /* FNIFI_UTILS_SYNCDIRECTORY_HPP */
