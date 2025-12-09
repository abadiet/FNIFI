#ifndef FNIFI_CONNECTION_SMB_HPP
#define FNIFI_CONNECTION_SMB_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils/utils.hpp"

#ifdef ENABLE_SAMBA
#include <libsmbclient.h>
#include <condition_variable>
#include <mutex>
#endif  /* ENABLE_SAMBA */

#ifdef ENABLE_LIBSMB2
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#endif  /* ENABLE_LIBSMB2 */


namespace fnifi {
namespace connection {

class SMB : virtual public IConnection {
public:
    SMB(const std::string& server = "127.0.0.1", const std::string& share = "",
        const std::string& workgroup = "", const std::string& username = "",
        const std::string& password = "");
    ~SMB() override;
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
    bool download(const std::filesystem::path& from,
                  const std::filesystem::path& to) override;
    bool upload(const std::filesystem::path& from,
                const std::filesystem::path& to) override;
    void remove(const std::filesystem::path& filepath) override;
    void createDirs(const std::filesystem::path& path) override;
    std::string getName() const override;

private:

#ifdef ENABLE_SAMBA
    struct UserData {
        std::string server;
        std::string share;
        std::string workgroup;
        std::string username;
        std::string password;
    } _userdata;
    struct NextEntryData {
        struct Directory {
            SMBCFILE* smb;
            const std::filesystem::path path;
        };
        SMB* self;
        const bool recursive;
        const bool files;
        const bool folders;
        std::vector<Directory> dirs;
    };

    static void get_auth_data_with_context_fn(SMBCCTX* c, const char* srv,
                                              const char* shr, char* wg,
                                              int wglen, char* un, int unlen,
                                              char* pw, int pwlen);
    static const libsmb_file_info* nextEntry(void* data, std::string& absname);

    SMBCCTX* _ctx;
    std::string _path;
    static std::condition_variable _cv;
    static std::mutex _mtx;
#endif  /* ENABLE_SAMBA */

#ifdef ENABLE_LIBSMB2
    struct NextEntryData {
        struct Directory {
            struct smb2dir* smb;
            const std::filesystem::path path;
        };
        SMB* self;
        const bool recursive;
        const bool files;
        const bool folders;
        std::vector<Directory> dirs;
    };

    static const smb2dirent* nextEntry(void* data, std::string& absname);

    struct smb2_context* _ctx;
    const std::string _server;
    const std::string _share;
    const std::string _workgroup;
    const std::string _username;
    const std::string _password;
    bool _connected;
#endif  /* ENABLE_LIBSMB2 */

};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_SMB_HPP */
