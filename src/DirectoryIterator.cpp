#include "fnifi/connection/DirectoryIterator.hpp"
#include "fnifi/utils.hpp"
#include <sys/stat.h>


using namespace fnifi;
using namespace fnifi::connection;

DirectoryIterator::DirectoryIterator(
    const std::filesystem::recursive_directory_iterator& its, const char* path)
{
    for (const auto& it : its) {
        if (it.is_regular_file()) {
            struct stat fileStat;
            if (lstat(it.path().c_str(), &fileStat) == 0) {
                const auto name = std::filesystem::proximate(it.path(), path)
                    .string();
                _entries.insert(name);
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
        const auto name = std::filesystem::proximate(entry, path)
            .string();
        _entries.insert(name);
    }
}

std::unordered_set<std::string>::const_iterator DirectoryIterator::begin()
const
{
    return _entries.begin();
}

std::unordered_set<std::string>::const_iterator DirectoryIterator::end() const
{
    return _entries.end();
}

size_t DirectoryIterator::size() const {
    return _entries.size();
}
