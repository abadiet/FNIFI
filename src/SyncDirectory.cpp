#include "fnifi/utils/SyncDirectory.hpp"


using namespace fnifi;
using namespace fnifi::utils;

SyncDirectory::FileStream::FileStream(const SyncDirectory& sync,
                                      const std::filesystem::path& filepath,
                                      bool ate)
: _sync(sync), _abspath(sync.setupFileStream(filepath)), _relapath(filepath),
    _syncDisabled(false)
{
    setup(ate);
}

SyncDirectory::FileStream::~FileStream() {
    if (is_open()) {
        close();
    }
}

bool SyncDirectory::FileStream::pull() {
    if (!_syncDisabled) {
        DLOG("FileStream", this, "Pull")

        close();

        const auto hasChanged = _sync.pull(_abspath, _relapath);

        open(_abspath, std::ios::in | std::ios::out | std::ios::binary);

        return hasChanged;
    }
    return false;
}

void SyncDirectory::FileStream::push() {
    if (!_syncDisabled) {
        DLOG("FileStream", this, "Push")

        flush();

        /* fill the buffer */
        seekg(0, std::ios::end);
        const auto len = tellg();
        if (len > 0) {
            fileBuf_t buf(static_cast<size_t>(len), '\0');
            seekg(0);
            read(reinterpret_cast<char*>(&buf[0]), len);

            _sync.push(_relapath, buf);
        }
    }
}

void SyncDirectory::FileStream::disableSync(bool pull) {
    DLOG("FileStream", this, "Disable synchronization")

    _syncDisabled = true;

    if (pull) {
        this->pull();
    }
}

void SyncDirectory::FileStream::enableSync(bool push) {
    DLOG("FileStream", this, "Enable synchronization")

    _syncDisabled = false;

    if (push) {
        this->push();
    }
}

void SyncDirectory::FileStream::take(TempFile& file) {
    close();
    file.close();
    std::filesystem::rename(file.getPath(), _abspath);
    open(_abspath, std::ios::in | std::ios::out | std::ios::binary);
}

std::filesystem::path SyncDirectory::FileStream::getPath(bool relative) const {
    if (relative) {
        return _relapath;
    }
    return _abspath;
}

SyncDirectory::FileStream::FileStream(const std::filesystem::path& abspath,
                                      const std::filesystem::path& relapath,
                                      bool ate, const SyncDirectory& sync)
: _sync(sync), _abspath(abspath), _relapath(relapath), _syncDisabled(false)
{
    setup(ate);
}

void SyncDirectory::FileStream::setup(bool ate) {
    DLOG("FileStream", this, "Instanciation with absolute path " << _abspath
         << " and SyncDirectory " << &_sync)

    std::ios::openmode flags = std::ios::in | std::ios::out | std::ios::binary;
    if (ate) {
        flags |= std::ios::ate;
    }
    if (!std::filesystem::exists(_abspath)) {
        flags |= std::ios::trunc;
    }

    open(_abspath, flags);

    if (!is_open()) {
        std::ostringstream msg;
        msg << "Cannot open file " << _abspath;
        ELOG("FileStream", this, msg.str())
        throw std::runtime_error(msg.str());
    }
}

SyncDirectory::SyncDirectory(connection::IConnection* conn,
                             const std::filesystem::path& path)
: _conn(conn), _path(path)
{
    DLOG("SyncDirectory", this, "Instanciation for IConnection " << conn
         << " and path " << path)
}

SyncDirectory::FileStream SyncDirectory::open(
    const std::filesystem::path& filepath, bool ate) const
{
    const auto abspath = setupFileStream(filepath);

    return FileStream(abspath, filepath, ate, *this);
}

std::filesystem::path SyncDirectory::setupFileStream(
    const std::filesystem::path& filepath) const
{
    DLOG("SyncDirectory", this, "Setup directories and file for a FileStream "
         "instance")

    /* create the directories if needed */
    const auto abspath = _path / filepath;
    std::filesystem::create_directories(abspath.parent_path());
    _conn->createDirs(filepath.parent_path());

    pull(abspath, filepath);

    return abspath;
}

bool SyncDirectory::exists(const std::filesystem::path& filepath) const {
    return std::filesystem::exists(_path / filepath);
}

bool SyncDirectory::pull(const std::filesystem::path& abspath,
                         const std::filesystem::path& relapath) const
{
    if (_conn->exists(relapath)) {
        _conn->download(relapath, abspath);
        return true;
    }
    return false;
}

void SyncDirectory::push(const std::filesystem::path& relapath,
                         const fileBuf_t& buf) const
{
    _conn->write(relapath, buf);
}
