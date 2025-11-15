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

DirectoryIterator::DirectoryIterator(
    const std::filesystem::recursive_directory_iterator& its, const char* path)
{
    for (const auto& it : its) {
        if (it.is_regular_file()) {
            struct stat fileStat;
            if (lstat(it.path().string().c_str(), &fileStat) == 0) {
                const auto name = std::filesystem::proximate(it.path(), path)
                    .string();
                _entries.insert({name, fileStat.st_ctimespec});
            } else {
                ELOG("DirectoryIterator " << this
                     << " failed to get the metadata of " << it.path()
                     << ". This file is ignored.")
            }
        }
    }
}

DirectoryIterator::DirectoryIterator(const DirectoryIterator& dirit,
                                     const char* path)
{
    for (auto& entry : dirit._entries) {
        const auto name = std::filesystem::proximate(entry.path, path)
            .string();
        _entries.insert({name, entry.st_ctimespec});
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
