#ifndef FNIFI_FILE_COLLECTION_HPP
#define FNIFI_FILE_COLLECTION_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils.hpp"
#include <string>
#include <fstream>


namespace fnifi {
namespace file {

class Collection {
public:
    Collection(connection::IConnection* conn, const char* indexingPath,
               const char* storingPath);
    ~Collection();
    void index();
    fileId_t getFirstId() const;
    fileId_t getLastId() const;
    std::string getFilePath(fileId_t id);
    fileBuf_t preview(fileId_t id);
    fileBuf_t read(fileId_t id);

private:
    struct __attribute__((packed)) MapNode {
        size_t offset;
        unsigned short lenght;
    };

    std::string getPreviewFilePath(fileId_t id) const;

    connection::IConnection* _conn;
    const std::string _indexingPath;
    const std::string _storingPath;
    fileId_t _nFiles;
    std::fstream _offsets;
    std::fstream _filepaths;
};

}  /* namespace file */
}  /* namesapce fnifi */

#endif  /* FNIFI_FILE_COLLECTION_HPP */
