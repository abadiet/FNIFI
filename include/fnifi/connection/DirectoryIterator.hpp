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
    DirectoryIterator();
    DirectoryIterator(const std::filesystem::recursive_directory_iterator& its,
                      bool files, bool folders);
    DirectoryIterator(const std::filesystem::directory_iterator& its,
                      bool files, bool folders);
    DirectoryIterator(void* data, const std::function<
                      const libsmb_file_info*(void*,std::string&)>& nextEntry);
    DirectoryIterator(const DirectoryIterator& dirit, const char* path);
    std::unordered_set<std::string>::const_iterator begin() const;
    std::unordered_set<std::string>::const_iterator end() const;
    size_t size() const;

private:
    std::unordered_set<std::string> _entries;
};

}  /* nanemsapce connection */
}  /* nanemsapce fnifi */

#endif  /* FNIFI_CONNECTION_DIRECTORYITERATOR */
