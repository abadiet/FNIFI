#include "fnifi/connection/DirectoryIterator.hpp"
#include "fnifi/utils.hpp"
#include <sys/stat.h>


using namespace fnifi;
using namespace fnifi::connection;

DirectoryIterator::DirectoryIterator() {}

DirectoryIterator::DirectoryIterator(
    const std::filesystem::recursive_directory_iterator& its, bool files,
    bool folders)
{
    for (const auto& it : its) {
        if (files && it.is_regular_file()) {
            _entries.insert(it.path());
        } else {
            if (folders && it.is_directory()) {
                _entries.insert(it.path());
            }
        }
    }
}

DirectoryIterator::DirectoryIterator(
    const std::filesystem::directory_iterator& its, bool files,
    bool folders)
{
    for (const auto& it : its) {
        if (files && it.is_regular_file()) {
            _entries.insert(it.path());
        } else {
            if (folders && it.is_directory()) {
                _entries.insert(it.path());
            }
        }
    }
}
DirectoryIterator::DirectoryIterator(void* data, const std::function<
    const libsmb_file_info*(void*, std::string&)>& nextEntry)
{
    std::string name;
    auto entry = nextEntry(data, name);
    while (entry != nullptr) {
        _entries.insert(name);
        entry = nextEntry(data, name);
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
