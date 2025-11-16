#include "fnifi/connection/DirectoryIterator.hpp"
#include <sys/stat.h>


using namespace fnifi;
using namespace fnifi::connection;

DirectoryIterator::DirectoryIterator() {
}

DirectoryIterator::DirectoryIterator(
    const std::filesystem::recursive_directory_iterator& its, const char* path)
{
    for (const auto& it : its) {
        if (it.is_regular_file()) {
            const auto name = std::filesystem::proximate(it.path(), path)
                .string();
            _entries.insert(name);
        }
    }
}

DirectoryIterator::DirectoryIterator(
    const std::unordered_set<std::string>& entries)
: _entries(entries)
{
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
