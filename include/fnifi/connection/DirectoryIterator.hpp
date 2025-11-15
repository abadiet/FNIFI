#ifndef FNIFI_CONNECTION_DIRECTORYITERATOR
#define FNIFI_CONNECTION_DIRECTORYITERATOR

#include <string>
#include <unordered_set>
#include <filesystem>
#include <ctime>


namespace fnifi {
namespace connection {

class DirectoryIterator {
public:
    struct Entry {
        const std::string path;
        const struct timespec st_ctimespec;
        bool operator==(const Entry& other) const;
    };

private:
    struct EntryHash {
        size_t operator()(
            const fnifi::connection::DirectoryIterator::Entry& obj) const;
    };

public:
    DirectoryIterator(const std::filesystem::recursive_directory_iterator& its,
                      const char* path);
    DirectoryIterator(const DirectoryIterator& dirit, const char* path);
    std::unordered_set<Entry, EntryHash>::const_iterator begin() const;
    std::unordered_set<Entry, EntryHash>::const_iterator end() const;
    size_t size() const;

private:
    std::unordered_set<Entry, EntryHash> _entries;
};

}  /* nanemsapce connection */
}  /* nanemsapce fnifi */

#endif  /* FNIFI_CONNECTION_DIRECTORYITERATOR */
