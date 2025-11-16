#ifndef FNIFI_CONNECTION_SMB_HPP
#define FNIFI_CONNECTION_SMB_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils.hpp"
#include <libsmbclient.h>


namespace fnifi {
namespace connection {

class SMB : virtual public IConnection {
public:
    SMB(const char* server = "127.0.0.1", const char* share = "",
        const char* workgroup = "", const char* username = "",
        const char* password = "");
    ~SMB() override;
    void connect() override;
    void disconnect(bool agressive = false) override;
    DirectoryIterator iterate(const char* path, bool recursive = true,
                              bool files = true, bool folders = false
                              ) override;
    bool exists(const char* filepath) override;
    struct stat getStats(const char* filepath) override;
    fileBuf_t read(const char* filepath) override;
    void write(const char* filepath, const fileBuf_t& buffer) override;
    void download(const char* from, const char* to) override;
    void upload(const char* from, const char* to) override;
    void remove(const char* filepath) override;

private:
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
            const std::string path;
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
};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_SMB_HPP */
