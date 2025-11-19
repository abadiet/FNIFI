#include "fnifi/connection/SMB.hpp"
#include "fnifi/utils/utils.hpp"

#ifdef _WIN32
#include <winnt.h>
#else  /* _WIN32 */
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_REPARSE_POINT 0x400
#endif  /* _WIN32 */
#define DOS_ISDIR(dos) ((dos & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define DOS_ISREG(dos)                                                        \
    ((dos & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0)

#define BUFFER_SZ 4096


using namespace fnifi;
using namespace fnifi::connection;

SMB::SMB(const std::string& server, const std::string& share,
         const std::string& workgroup, const std::string& username,
         const std::string& password)
: _userdata({server, share, workgroup, username, password}), _ctx(nullptr)
{
    if (server == "") {
        throw std::runtime_error("Trying to instantiate a SMB object without "
                                 "providing a server");
    }
    std::ostringstream oss;
    oss << "smb://" << server;
    if (share != "") {
        oss << "/" << share;
    }
    oss << "/";
    _path = oss.str();
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

DirectoryIterator SMB::iterate(const std::filesystem::path& path,
                               bool recursive, bool files, bool folders)
{
    const auto fullpath = _path + path.string();
    auto rootdir = smbc_getFunctionOpendir(_ctx)(_ctx, fullpath.c_str());
    if (!rootdir) {
        ELOG("SMB " << this << " failed to open the directory " << fullpath
             << ": " << strerror(errno))
        return DirectoryIterator();
    }

    NextEntryData data = {this, recursive, files, folders, {{rootdir, path}}};
    DirectoryIterator iter(&data, nextEntry);

    /* note that rootdir has been close by DirectoryIterator */

    return iter;
}

bool SMB::exists(const std::filesystem::path& filepath) {
    /* TODO: a bit dirty */
    struct stat filestat;
    const auto path = _path + filepath.string();
    return smbc_getFunctionStat(_ctx)(_ctx, path.c_str(), &filestat) == 0;
}

struct stat SMB::getStats(const std::filesystem::path& filepath) {
    struct stat filestat;
    const auto path = _path + filepath.string();
    if (smbc_getFunctionStat(_ctx)(_ctx, path.c_str(), &filestat) != 0) {
        ELOG("SMB " << this << " failed to get the stat of " << path
             << ": " << strerror(errno))
    }
    return filestat;
}

fileBuf_t SMB::read(const std::filesystem::path& filepath) {
    fileBuf_t res;
    const auto path = _path + filepath.string();
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

void SMB::write(const std::filesystem::path& filepath, const fileBuf_t& buffer)
{
    const auto path = _path + filepath.string();
    auto file = smbc_getFunctionOpen(_ctx)(_ctx, path.c_str(), O_WRONLY |
                                           O_TRUNC | O_CREAT, 0);
    if (!file) {
        ELOG("SMB " << this << " failed to open " << path << ": "
             << strerror(errno))
        return;
    }

    if (buffer.size() > 0) {
        const auto len = smbc_getFunctionWrite(_ctx)(_ctx, file, buffer.data(),
                                                     buffer.size());
        if (len < 0 || static_cast<size_t>(len) != buffer.size()) {
            ELOG("SMB " << this << " failed to write to " << path
                 << ": " << strerror(errno))
        }
    }

    if (smbc_getFunctionClose(_ctx)(_ctx, file) != 0) {
        ELOG("SMB " << this << " failed to close " << path << ": "
             << strerror(errno))
    }
}

void SMB::download(const std::filesystem::path& from,
                   const std::filesystem::path& to)
{
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

void SMB::upload(const std::filesystem::path& from,
                 const std::filesystem::path& to)
{
    std::ifstream file(from, std::ios::ate);
    const auto len = file.tellg();
    fileBuf_t buf(static_cast<size_t>(len), '\0');
    file.seekg(0);
    file.read(reinterpret_cast<char*>(&buf[0]), len);
    write(to, buf);
    file.close();
}

void SMB::remove(const std::filesystem::path& filepath) {
    const auto path = _path + filepath.string();
    if (smbc_getFunctionUnlink(_ctx)(_ctx, path.c_str()) != 0) {
        ELOG("SMB " << this << " failed to remove " << path)
    }
}

void SMB::createDirs(const std::filesystem::path& path) {
    const auto dirpath = _path + path.string();
    if (smbc_getFunctionMkdir(_ctx)(_ctx, dirpath.c_str(), 0) != 0) {
        ELOG("SMB " << this << " failed to create directories " << path)
    }
}

std::string SMB::getName() const {
    return _path;
}

void SMB::get_auth_data_with_context_fn(SMBCCTX* c, const char* srv,
                                        const char* shr, char* wg, int wglen,
                                        char* un, int unlen, char* pw,
                                        int pwlen)
{
    auto userdata = static_cast<UserData*>(smbc_getOptionUserData(c));

    if (userdata->server == srv &&
        userdata->share == shr)
    {
        if (userdata->workgroup != "") {
            std::strncpy(wg, userdata->workgroup.data(),
                         static_cast<size_t>(wglen - 1));
        }
        if (userdata->username != "") {
            std::strncpy(un, userdata->username.data(),
                         static_cast<size_t>(unlen - 1));
        }
        if (userdata->password != "") {
            std::strncpy(pw, userdata->password.data(),
                         static_cast<size_t>(pwlen - 1));
        }
    }
}

const libsmb_file_info* SMB::nextEntry(void* data, std::string& absname) {
    auto d = reinterpret_cast<NextEntryData*>(data);

    auto entry = smbc_getFunctionReaddirPlus(d->self->_ctx)(
        d->self->_ctx, d->dirs.back().smb);
    while (
        entry != nullptr &&
        !(d->files && DOS_ISREG(entry->attrs)) &&
        !(
            (d->folders || d->recursive) && DOS_ISDIR(entry->attrs) &&
            (std::strcmp(entry->name, ".") != 0) &&
            (std::strcmp(entry->name, "..") != 0)
        )
    ) {
        entry = smbc_getFunctionReaddirPlus(d->self->_ctx)(d->self->_ctx,
                                                           d->dirs.back().smb);
    }

    if (entry == nullptr) {
        /* end of the current directory */
        if (d->dirs.size() > 0) {
            smbc_getFunctionClosedir(d->self->_ctx)(d->self->_ctx,
                                                    d->dirs.back().smb);
            d->dirs.pop_back();

            if (d->dirs.size() > 0) {
                /* this was not the root dir */
                return nextEntry(data, absname);
            }
        }

        /* the process iterated over every single files */
        return nullptr;
    }

    /* fix entry's name to be absolute */
    absname = d->dirs.back().path / entry->name;

    if (d->recursive && DOS_ISDIR(entry->attrs)) {
        /* new directory */
        const auto fullpath = d->self->_path + absname;
        auto dir = smbc_getFunctionOpendir(d->self->_ctx)(d->self->_ctx,
                                                          fullpath.c_str());
        if (!dir) {
            ELOG("SMB " << d->self << " failed to open the directory "
                 << absname << ": " << strerror(errno))
        }
        d->dirs.push_back({dir, absname});

        if (!d->folders) {
            /* we do not care of folders */
            return nextEntry(data, absname);
        }
    }

    return entry;
}
