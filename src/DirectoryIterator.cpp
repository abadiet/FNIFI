#include "fnifi/connection/DirectoryIterator.hpp"
#include "fnifi/utils.hpp"
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
    for (const auto& entry : entries) {
        addEntry(entry, files, folders);
    }
}

DirectoryIterator::DirectoryIterator(
    const std::filesystem::directory_iterator& entries, bool files,
    bool folders)
{
    for (const auto& entry : entries) {
        addEntry(entry, files, folders);
    }
}
DirectoryIterator::DirectoryIterator(void* data, const std::function<
    const libsmb_file_info*(void*, std::string&)>& nextEntry)
{
    std::string name;
    auto entry = nextEntry(data, name);
    while (entry != nullptr) {
        _entries.insert({name, entry->ctime_ts});
        entry = nextEntry(data, name);
    }
}

DirectoryIterator::DirectoryIterator(const DirectoryIterator& dirit,
                                     const std::filesystem::path& path)
{
    for (auto& entry : dirit._entries) {
        const auto name = std::filesystem::proximate(entry.path, path)
            .string();
        _entries.insert({name, entry.ctime});
    }
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
    if ((files && entry.is_regular_file()) || (folders && entry.is_directory())
    ) {
        struct stat fileStat;
        if (lstat(entry.path().c_str(), &fileStat) == 0) {
            _entries.insert({entry.path(), fileStat.st_ctimespec});
        } else {
            ELOG("DirectoryIterator " << this
                 << " failed to get the metadata of " << entry.path()
                 << ". This file is ignored.")
        }
    }
}
