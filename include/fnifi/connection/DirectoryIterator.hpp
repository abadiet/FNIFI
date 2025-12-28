#ifndef FNIFI_CONNECTION_DIRECTORYITERATOR
#define FNIFI_CONNECTION_DIRECTORYITERATOR

#include <string>
#include <unordered_set>
#include <filesystem>
#include <ctime>
#include <functional>
#ifdef ENABLE_SAMBA
#include <libsmbclient.h>
#endif  /* ENABLE_SAMBA */
#ifdef ENABLE_LIBSMB2
#include <smb2/smb2.h>
#include <smb2/libsmb2.h>
#endif  /* ENABLE_LIBSMB2 */


namespace fnifi {
namespace connection {

class DirectoryIterator {
public:
    struct Entry {
        const std::string path;
        const struct timespec mtime;
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
#ifdef ENABLE_SAMBA
    DirectoryIterator(void* data, const std::function<
                      const libsmb_file_info*(void*,std::string&)>& nextEntry);
#endif  /* ENABLE_SAMBA */
#ifdef ENABLE_LIBSMB2
    DirectoryIterator(void* data, const std::function<
                      const smb2dirent*(void*,std::string&)>& nextEntry);
#endif  /* ENABLE_LIBSMB2 */
    DirectoryIterator(const DirectoryIterator& dirit,
                      const std::filesystem::path& path);
    std::unordered_set<Entry, EntryHash>::const_iterator begin() const;
    std::unordered_set<Entry, EntryHash>::const_iterator end() const;
    size_t size() const;

private:
    /**
     * TODO: to move in connection::Local
     */
    void addEntry(const std::filesystem::directory_entry& entry, bool files,
                  bool folders);
    std::unordered_set<Entry, EntryHash> _entries;
};

}  /* nanemsapce connection */
}  /* nanemsapce fnifi */

#endif  /* FNIFI_CONNECTION_DIRECTORYITERATOR */
