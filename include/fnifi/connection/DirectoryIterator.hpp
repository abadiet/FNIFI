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
    DirectoryIterator(const std::filesystem::recursive_directory_iterator& its,
                      const char* path);
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
