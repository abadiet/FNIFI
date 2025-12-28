#include "fnifi/connection/DirectoryIterator.hpp"
#include "fnifi/utils/utils.hpp"
#include <sys/stat.h>


using namespace fnifi;
using namespace fnifi::connection;

bool DirectoryIterator::Entry::operator==(
    const DirectoryIterator::Entry& other) const
{
    return path == other.path;
}

size_t DirectoryIterator::EntryHash::operator()(
    const DirectoryIterator::Entry& obj) const
{
    return std::hash<std::string>{}(obj.path);
}

DirectoryIterator::DirectoryIterator() {}

DirectoryIterator::DirectoryIterator(
    const std::filesystem::recursive_directory_iterator& entries, bool files,
    bool folders)
{
    DLOG("DirectoryIterator", this, "Instanciation via "
         "std::filesystem::recursive_directory_iterator")

    for (const auto& entry : entries) {
        addEntry(entry, files, folders);
    }

    ILOG("DirectoryIterator", this, "Found " << _entries.size() << " elements")
}

DirectoryIterator::DirectoryIterator(
    const std::filesystem::directory_iterator& entries, bool files,
    bool folders)
{
    DLOG("DirectoryIterator", this, "Instanciation via "
         "std::filesystem::directory_iterator")

    for (const auto& entry : entries) {
        addEntry(entry, files, folders);
    }

    ILOG("DirectoryIterator", this, "Found " << _entries.size() << " elements")
}

#ifdef ENABLE_SAMBA
DirectoryIterator::DirectoryIterator(void* data, const std::function<
    const libsmb_file_info*(void*, std::string&)>& nextEntry)
{
    DLOG("DirectoryIterator", this, "Instanciation via SMB callback")

    std::string name;
    auto entry = nextEntry(data, name);
    while (entry != nullptr) {
        _entries.insert({name, entry->mtime_ts});
        entry = nextEntry(data, name);
    }

    ILOG("DirectoryIterator", this, "Found " << _entries.size() << " elements")
}
#endif  /* ENABLE_SAMBA */

#ifdef ENABLE_LIBSMB2
DirectoryIterator::DirectoryIterator(void* data, const std::function<
    const smb2dirent*(void*, std::string&)>& nextEntry)
{
    DLOG("DirectoryIterator", this, "Instanciation via SMB callback")

    std::string name;
    auto entry = nextEntry(data, name);
    while (entry != nullptr) {
        _entries.insert({name, {
            .tv_sec = static_cast<time_t>(entry->st.smb2_mtime),
            .tv_nsec = static_cast<long>(entry->st.smb2_mtime_nsec),
        }});
        entry = nextEntry(data, name);
    }

    ILOG("DirectoryIterator", this, "Found " << _entries.size() << " elements")
}
#endif  /* ENABLE_LIBSMBS2 */

DirectoryIterator::DirectoryIterator(const DirectoryIterator& dirit,
                                     const std::filesystem::path& path)
{
    DLOG("DirectoryIterator", this, "Instanciation via DirectoryIterator with "
         "path " << path)

    for (auto& entry : dirit._entries) {
        /* TODO: std::fs::relative is a pretty slow function */
        const auto name = std::filesystem::relative(entry.path, path)
            .string();
        _entries.insert({name, entry.mtime});
    }

    ILOG("DirectoryIterator", this, "Found " << _entries.size() << " elements")
}

std::unordered_set<DirectoryIterator::Entry, DirectoryIterator::EntryHash>
    ::const_iterator DirectoryIterator::begin() const
{
    return _entries.begin();
}

std::unordered_set<DirectoryIterator::Entry, DirectoryIterator::EntryHash>
    ::const_iterator DirectoryIterator::end() const
{
    return _entries.end();
}

size_t DirectoryIterator::size() const {
    return _entries.size();
}

void DirectoryIterator::addEntry(const std::filesystem::directory_entry& entry,
                                 bool files, bool folders)
{
    if (((files && entry.is_regular_file()) ||
            (folders && entry.is_directory())
        ) &&
        !entry.path().string().starts_with('.')
    ) {
        struct stat fileStat;
        if (lstat(entry.path().c_str(), &fileStat) == 0) {
            _entries.insert({entry.path(), fileStat.st_mtimespec});
        } else {
            WLOG("DirectoryIterator", this, "Failed to get the metadata of "
                 << entry.path() << ": this file is ignored")
        }
    }
}
