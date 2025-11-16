#ifndef FNIFI_CONNECTION_DIRECTORYITERATOR
#define FNIFI_CONNECTION_DIRECTORYITERATOR

#include <string>
#include <unordered_set>
#include <filesystem>
#include <ctime>
#include <libsmbclient.h>
#include <functional>


namespace fnifi {
namespace connection {

class DirectoryIterator {
public:
    struct Entry {
        const std::string path;
        const struct timespec ctime;
        bool operator==(const Entry& other) const;
    };

private:
    struct EntryHash {
        size_t operator()(
            const fnifi::connection::DirectoryIterator::Entry& obj) const;
    };

public:
    DirectoryIterator();
    DirectoryIterator(
        const std::filesystem::recursive_directory_iterator& entries,
        bool files, bool folders);
    DirectoryIterator(const std::filesystem::directory_iterator& entries,
                      bool files, bool folders);
    DirectoryIterator(void* data, const std::function<
                      const libsmb_file_info*(void*,std::string&)>& nextEntry);
    DirectoryIterator(const DirectoryIterator& dirit, const char* path);
    std::unordered_set<Entry, EntryHash>::const_iterator begin() const;
    std::unordered_set<Entry, EntryHash>::const_iterator end() const;
    size_t size() const;

private:
    void addEntry(const std::filesystem::directory_entry& entry, bool files,
                  bool folders);
    std::unordered_set<Entry, EntryHash> _entries;
};

}  /* nanemsapce connection */
}  /* nanemsapce fnifi */

#endif  /* FNIFI_CONNECTION_DIRECTORYITERATOR */
