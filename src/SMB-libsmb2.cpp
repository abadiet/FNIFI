#include "fnifi/connection/SMB.hpp"
#include "fnifi/utils/utils.hpp"
#include <fcntl.h>

#define BUFFER_SZ 4096


using namespace fnifi;
using namespace fnifi::connection;

SMB::SMB(const std::string& server, const std::string& share,
         const std::string& username, const std::string& password)
: _ctx(nullptr), _server(server), _share(share), _username(username),
    _password(password), _connected(false)
{
    DLOG("SMB", this, "Instanciation for server \"" << server << "\" and share"
         " \"" << share << "\"")

    _ctx = smb2_init_context();
    if (!_ctx) {
        const auto msg("Cannot setup the SMB context");
        ELOG("SMB", this, msg)
        throw std::runtime_error(msg);
    }
}

SMB::~SMB() {
    disconnect();
    smb2_close_context(_ctx);
    smb2_destroy_context(_ctx);
}

void SMB::connect() {
    DLOG("SMB", this, "Connection")

    if (_connected) {
        return;
    }

    smb2_set_password(_ctx, _password.c_str());
    const auto res = smb2_connect_share(_ctx, _server.c_str(), _share.c_str(),
                                        _username.c_str());
    if (res) {
        std::ostringstream msg;
        msg << "Connection failed: " << smb2_get_error(_ctx);
        ELOG("SMB", this, msg.str())
        throw std::runtime_error(msg.str());
    }

    _connected = true;

    /* TODO: check if connected */
}

void SMB::disconnect(bool aggresive) {
    if (smb2_context_active(_ctx)) { /* TODO: return 0 if active? */
        const auto res = smb2_disconnect_share(_ctx);
        if (res) {
            WLOG("SMB", this, "Disonnection failed: " << smb2_get_error(_ctx))
        }
    }

    _connected = false;

    UNUSED(aggresive)
}

DirectoryIterator SMB::iterate(const std::filesystem::path& path,
                               bool recursive, bool files, bool folders)
{
    DLOG("SMB", this, "Iteration over path " << path)

    auto rootdir = smb2_opendir(_ctx, path.c_str());
    if (!rootdir) {
        WLOG("SMB", this, "Failed to open the directory " << path
             << ": will return an empty iterator. More: "
             << smb2_get_error(_ctx))
        return DirectoryIterator();
    }

    NextEntryData data = {this, recursive, files, folders, {{rootdir, path}}};
    DirectoryIterator iter(&data, nextEntry);

    /* note that rootdir has been close by DirectoryIterator */

    return iter;
}

bool SMB::exists(const std::filesystem::path& filepath) {
    DLOG("SMB", this, "Existance check of " << filepath)

    /* TODO: a bit dirty */
    smb2_stat_64 fileStat;

    return (smb2_stat(_ctx, filepath.c_str(), &fileStat) == 0);
}

struct stat SMB::getStats(const std::filesystem::path& filepath) {
    DLOG("SMB", this, "Get statistics for " << filepath)

    smb2_stat_64 fileStat;

    if (smb2_stat(_ctx, filepath.c_str(), &fileStat) != 0) {
        WLOG("SMB", this, "Failed to get the stat of " << filepath
             << ": will return the default instanciated stat. More: "
             << smb2_get_error(_ctx))
        fileStat.smb2_size = 0;
    }

    struct stat res;
    res.st_nlink = static_cast<nlink_t>(fileStat.smb2_nlink);
    res.st_ino = fileStat.smb2_ino;
    res.st_size = static_cast<off_t>(fileStat.smb2_size);
    res.st_atimespec = {
        .tv_sec = static_cast<__darwin_time_t>(fileStat.smb2_atime),
        .tv_nsec = static_cast<long>(fileStat.smb2_atime_nsec),
    };
    res.st_mtimespec = {
        .tv_sec = static_cast<__darwin_time_t>(fileStat.smb2_mtime),
        .tv_nsec = static_cast<long>(fileStat.smb2_mtime_nsec),
    };
    res.st_ctimespec = {
        .tv_sec = static_cast<__darwin_time_t>(fileStat.smb2_ctime),
        .tv_nsec = static_cast<long>(fileStat.smb2_ctime_nsec),
    };
    return res;
}

fileBuf_t SMB::read(const std::filesystem::path& filepath) {
    DLOG("SMB", this, "Read file " << filepath)

    fileBuf_t res;

    auto file = smb2_open(_ctx, filepath.c_str(), O_RDONLY);
    if (!file) {
        WLOG("SMB", this, "Failed to open " << filepath
             << ": will return an empty buffer. More: "
             << smb2_get_error(_ctx))
        return res;
    }

    uint8_t buf[BUFFER_SZ];
    auto len = smb2_read(_ctx, file, buf, BUFFER_SZ);
    while (len > 0) {
        res.insert(res.end(), buf, buf + len);
        len = smb2_read(_ctx, file, buf, BUFFER_SZ);
    }
    if (len < 0) {
        WLOG("SMB", this, "Failed to read " << filepath
             << ": will return the potentially corrupted buffer. More: "
             << smb2_get_error(_ctx))
    }

    if (smb2_close(_ctx, file) != 0) {
        WLOG("SMB", this, "Failed to close " << filepath << ". More: "
             << smb2_get_error(_ctx))
    }

    return res;
}

void SMB::write(const std::filesystem::path& filepath, const fileBuf_t& buffer)
{
    DLOG("SMB", this, "Write to file " << filepath)

    auto file = smb2_open(_ctx, filepath.c_str(),  O_WRONLY | O_TRUNC |
                          O_CREAT);
    if (!file) {
        WLOG("SMB", this, "Failed to open " << filepath
             << ". More: " << smb2_get_error(_ctx))
        return;
    }

    if (buffer.size() > 0) {
        const auto len = smb2_write(_ctx, file, buffer.data(),
                                    static_cast<uint32_t>(buffer.size()));
        if (len < 0 || static_cast<size_t>(len) != buffer.size()) {
            WLOG("SMB", this, "Failed to write to " << filepath << ". More: "
                 << smb2_get_error(_ctx))
        }
    }

    if (smb2_close(_ctx, file) != 0) {
        WLOG("SMB", this, "Failed to close " << filepath << ". More: "
             << smb2_get_error(_ctx))
    }
}

bool SMB::download(const std::filesystem::path& from,
                   const std::filesystem::path& to)
{
    DLOG("SMB", this, "Download from " << from << " to " << to)

    const auto content = read(from);
    if (content.size() == 0) {
        return false;
    }

    std::ofstream file(to, std::ios::trunc);
    if (!file.is_open()) {
        WLOG("SMB", this, "Failed to open " << to)
        return false;
    }
    file.write(reinterpret_cast<const char*>(content.data()),
               std::streamsize(content.size()));
    file.close();

    return true;
}

bool SMB::upload(const std::filesystem::path& from,
                 const std::filesystem::path& to)
{
    DLOG("SMB", this, "Upload from " << from << " to " << to)

    std::ifstream file(from, std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    const auto len = file.tellg();
    fileBuf_t buf(static_cast<size_t>(len), '\0');
    file.seekg(0);
    file.read(reinterpret_cast<char*>(&buf[0]), len);
    write(to, buf);
    file.close();

    return true;
}

void SMB::remove(const std::filesystem::path& filepath) {
    DLOG("SMB", this, "Remove file " << filepath)

    if (smb2_unlink(_ctx, filepath.c_str()) != 0) {
        WLOG("SMB", this, "Failed to remove " << filepath << ". More: "
             << smb2_get_error(_ctx))
    }
}

void SMB::createDirs(const std::filesystem::path& path) {
    DLOG("SMB", this, "Create directories for path " << path)

    std::filesystem::path dirs;

    for (const auto& dir : path) {
        dirs /= dir;
        /* TODO: check if dir exists to avoid a useless warning */
        if (smb2_mkdir(_ctx, dirs.c_str()) != 0) {
            WLOG("SMB", this, "Failed to create directories " << path
                 << ". More: " << smb2_get_error(_ctx))
        }
    }
}

std::string SMB::getName() const {
    std::ostringstream oss;
    oss << "smb://" << _server;
    if (_share != "") {
        oss << "/" << _share;
    }
    oss << "/";
    return oss.str();
}

const smb2dirent* SMB::nextEntry(void* data, std::string& absname) {
    auto d = reinterpret_cast<NextEntryData*>(data);

    auto entry = smb2_readdir(d->self->_ctx, d->dirs.back().smb);
    while (
        entry != nullptr &&
        !(d->files && (entry->st.smb2_type == SMB2_TYPE_FILE)) &&
        !(
            (d->folders || d->recursive) &&
            (entry->st.smb2_type == SMB2_TYPE_DIRECTORY) &&
            (std::strcmp(entry->name, ".") != 0) &&
            (std::strcmp(entry->name, "..") != 0)
        )
    ) {
        entry = smb2_readdir(d->self->_ctx, d->dirs.back().smb);
    }

    if (entry == nullptr) {
        /* end of the current directory */
        if (d->dirs.size() > 0) {
            smb2_closedir(d->self->_ctx, d->dirs.back().smb);
            d->dirs.pop_back();

            if (d->dirs.size() > 0) {
                /* this was not the root dir */
                return nextEntry(data, absname);
            }
        }
        /* the process iterated over every single files */
        return nullptr;
    }

    std::cout << entry->name << std::endl;

    /* fix entry's name to be absolute */
    absname = d->dirs.back().path / entry->name;

    if (d->recursive && (entry->st.smb2_type == SMB2_TYPE_DIRECTORY)) {
        /* new directory */
        auto dir = smb2_opendir(d->self->_ctx, absname.c_str());
        if (!dir) {
            WLOG("SMB", d->self, "Failed to open the directory " << absname
                 << ": will ignore this directory. More: "
                 << smb2_get_error(d->self->_ctx))
            return nextEntry(data, absname);
        }

        d->dirs.push_back({dir, absname});

        if (!d->folders) {
            /* we do not care of folders */
            return nextEntry(data, absname);
        }
    }

    return entry;
}

