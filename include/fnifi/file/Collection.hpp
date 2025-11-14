#ifndef FNIFI_FILE_COLLECTION_HPP
#define FNIFI_FILE_COLLECTION_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/file/File.hpp"
#include "fnifi/utils.hpp"
#include <unordered_set>
#include <string>
#include <fstream>
#include <filesystem>
#include <time.h>


namespace fnifi {
namespace file {

class Collection : virtual public IFileHelper {
public:
    Collection(connection::IConnection* conn, const char* indexingPath,
               const char* storingPath);
    ~Collection() override;
    void index(std::unordered_set<fileId_t>& removed,
               std::unordered_set<fileId_t>& added);
    void defragment();
    std::string getFilePath(fileId_t id) override;
    fileBuf_t preview(fileId_t id) override;
    fileBuf_t read(fileId_t id) override;
    std::unordered_set<File>::const_iterator begin() const;
    std::unordered_set<File>::const_iterator end() const;
    size_t size() const;

private:
    using offset_t = size_t;
    using lenght_t = unsigned short;
    struct __attribute__((packed)) MapNode {
        offset_t offset;
        lenght_t lenght;
    };
    struct __attribute__((packed)) Info {
        struct timespec lastIndexing = {0, 0};
    };

    std::string getPreviewFilePath(fileId_t id) const;

    std::unordered_set<File> _files;
    connection::IConnection* _conn;
    const std::filesystem::path _indexingPath;
    const std::filesystem::path _storingPath;
    std::fstream _mapping;
    std::fstream _filepaths;
    std::unordered_set<fileId_t> _availableIds;
};

}  /* namespace file */
}  /* namesapce fnifi */

#endif  /* FNIFI_FILE_COLLECTION_HPP */
