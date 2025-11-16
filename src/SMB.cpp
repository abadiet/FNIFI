#include "fnifi/connection/SMB.hpp"
#include "fnifi/utils.hpp"

#define BUFFER_SZ 4096


using namespace fnifi;
using namespace fnifi::connection;

SMB::SMB(const char* server, const char* share, const char* workgroup,
         const char* username, const char* password)
: _userdata({server, share, workgroup, username, password}), _ctx(nullptr)
{
    if (!server) {
        throw std::runtime_error("Trying to instantiate a SMB object with "
                                 "server == nullptr");
    }
    std::ostringstream oss;
    oss << "smb://";
    /*
    if (username) {
        if (workgroup) {
            oss << workgroup << ";";
        }
        oss << username;
        if (password) {
            oss << ":" << password;
        }
        oss << "@";
    }
    */
    oss << server;
    if (share) {
        oss << "/" << share;
    }
    oss << "/";
    _path = oss.str();
    DLOG(_path)
}

SMB::~SMB() {
    disconnect();
}

void SMB::connect() {
    if (_ctx) {
        return;
    }

    _ctx = smbc_new_context();
    if (!_ctx) {
        throw std::runtime_error("Cannot setup the SMB context");
    }

#ifdef FNIFI_DEBUG
    smbc_setDebug(_ctx, 1);
#endif  /* FNIFI_DEBUG */
    smbc_setOptionUserData(_ctx, &_userdata);
    smbc_setFunctionAuthDataWithContext(_ctx,
                                        SMB::get_auth_data_with_context_fn);
    _ctx = smbc_init_context(_ctx);
    if (!_ctx) {
        throw std::runtime_error("Cannot setup the SMB context");
    }
}

void SMB::disconnect(bool aggresive) {
    if (_ctx) {
        smbc_free_context(_ctx, aggresive);
    }
}

DirectoryIterator SMB::iterate(const char* path) {
    /* TODO: recursive */
    const auto fullpath = _path + path;
    auto dir = smbc_getFunctionOpendir(_ctx)(_ctx, fullpath.c_str());
    if (!dir) {
        ELOG("SMB " << this << " failed to open the directory " << fullpath
             << ": " << strerror(errno))
        return DirectoryIterator();
    }

    std::unordered_set<std::string> filepaths;
    const std::string pathstr(path);
    auto entry = smbc_getFunctionReaddir(_ctx)(_ctx, dir);
    while (entry != nullptr) {
        if (entry->smbc_type == SMBC_FILE) {
            filepaths.insert(pathstr + "/" + entry->name);
        }
        entry = smbc_getFunctionReaddir(_ctx)(_ctx, dir);
    }

    smbc_getFunctionClosedir(_ctx)(_ctx, dir);

    return DirectoryIterator(filepaths);
}

bool SMB::exists(const char* filepath) {
    /* TODO: a bit dirty */
    struct stat filestat;
    const auto path = _path + filepath;
    return smbc_getFunctionStat(_ctx)(_ctx, path.c_str(), &filestat) == 0;
}

struct stat SMB::getStats(const char* filepath) {
    struct stat filestat;
    const auto path = _path + filepath;
    if (smbc_getFunctionStat(_ctx)(_ctx, path.c_str(), &filestat) != 0) {
        ELOG("SMB " << this << " failed to get the stat of " << path
             << ": " << strerror(errno))
    }
    return filestat;
}

fileBuf_t SMB::read(const char* filepath) {
    fileBuf_t res;
    const auto path = _path + filepath;
    auto file = smbc_getFunctionOpen(_ctx)(_ctx, path.c_str(), O_RDONLY, 0);
    if (!file) {
        ELOG("SMB " << this << " failed to open " << path << ": "
             << strerror(errno))
        return res;
    }

    char buf[BUFFER_SZ];
    auto len = smbc_getFunctionRead(_ctx)(_ctx, file, buf, BUFFER_SZ);
    while (len > 0) {
        res.insert(res.end(), buf, buf + len);
        len = smbc_getFunctionRead(_ctx)(_ctx, file, buf, BUFFER_SZ);
    }
    if (len < 0) {
        ELOG("SMB " << this << " failed to read " << path << ": "
             << strerror(errno))
    }

    if (smbc_getFunctionClose(_ctx)(_ctx, file) != 0) {
        ELOG("SMB " << this << " failed to close " << path << ": "
             << strerror(errno))
    }

    return res;
}

void SMB::write(const char* filepath, const fileBuf_t& buffer) {
    const auto path = _path + filepath;
    auto file = smbc_getFunctionOpen(_ctx)(_ctx, path.c_str(), O_WRONLY |
                                           O_TRUNC | O_CREAT, 0);
    if (!file) {
        ELOG("SMB " << this << " failed to open " << path << ": "
             << strerror(errno))
        return;
    }

    const auto len = smbc_getFunctionWrite(_ctx)(_ctx, file, buffer.data(),
                                                 buffer.size());
    if (len < 0 || static_cast<size_t>(len) != buffer.size()) {
        ELOG("SMB " << this << " failed to write to " << path
             << ": " << strerror(errno))
    }

    if (smbc_getFunctionClose(_ctx)(_ctx, file) != 0) {
        ELOG("SMB " << this << " failed to close " << path << ": "
             << strerror(errno))
    }
}

void SMB::download(const char* from, const char* to) {
    const auto content = read(from);
    std::ofstream file(to, std::ios::trunc);
    if (!file.is_open()) {
        ELOG("SMB " << this << " failed to open " << to)
        return;
    }
    file.write(reinterpret_cast<const char*>(content.data()),
               std::streamsize(content.size()));
    file.close();
}

void SMB::upload(const char* from, const char* to) {
    std::ifstream file(from, std::ios::ate);
    const auto len = file.tellg();
    fileBuf_t buf(static_cast<size_t>(len), '\0');
    file.seekg(0);
    file.read(reinterpret_cast<char*>(&buf[0]), len);
    write(to, buf);
    file.close();
}

void SMB::remove(const char* filepath) {
    const auto path = _path + filepath;
    if (smbc_getFunctionUnlink(_ctx)(_ctx, path.c_str()) != 0) {
        ELOG("SMB " << this << " failed to remove " << path)
    }
}

void SMB::get_auth_data_with_context_fn(SMBCCTX* c, const char* srv,
                                        const char* shr, char* wg, int wglen,
                                        char* un, int unlen, char* pw,
                                        int pwlen)
{
    auto userdata = static_cast<UserData*>(smbc_getOptionUserData(c));

    if (std::strcmp(srv, userdata->server) == 0 &&
        std::strcmp(shr, userdata->share) == 0)
    {
        if (userdata->workgroup) {
            std::strncpy(wg, userdata->workgroup,
                         static_cast<size_t>(wglen - 1));
        }
        if (userdata->username) {
            std::strncpy(un, userdata->username,
                         static_cast<size_t>(unlen - 1));
        }
        if (userdata->password) {
            std::strncpy(pw, userdata->password,
                         static_cast<size_t>(pwlen - 1));
        }
    }
}
