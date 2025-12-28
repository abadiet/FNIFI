#ifndef FNIFI_UTILS_SYNCDIRECTORY_HPP
#define FNIFI_UTILS_SYNCDIRECTORY_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils/utils.hpp"
#include "fnifi/utils/TempFile.hpp"
#include <fstream>
#include <ctime>


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
                   const SyncDirectory& sync, struct timespec lastMTime);
        void setup(bool ate);

        const SyncDirectory& _sync;
        const std::filesystem::path _abspath;
        const std::filesystem::path _relapath;
        bool _syncDisabled;
        struct timespec _lastMtime;

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
        const std::filesystem::path& filepath, struct timespec& lastMTime,
        bool mkdir = true) const;
    struct timespec pull(const std::filesystem::path& abspath,
                         const std::filesystem::path& relapath,
                         const struct timespec& lastMTime) const;
    void push(const std::filesystem::path& relapath, const fileBuf_t& buf)
        const;

    connection::IConnection* _conn;
    const std::filesystem::path _path;
};

}  /* namesapce connection */
}  /* namesapce fnifi */

#endif  /* FNIFI_UTILS_SYNCDIRECTORY_HPP */
