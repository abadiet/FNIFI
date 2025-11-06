#ifndef FNIFI_COLLECTION_HPP
#define FNIFI_COLLECTION_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils.hpp"
#include <string>


namespace fnifi {

class Collection {
public:
    Collection(connection::IConnection* conn, const char* indexingPath,
               const char* storingPath);
    fileId_t getFirstId() const;
    fileId_t getLastId() const;
    std::string getFilePath(fileId_t id) const;
    fileBuf_t preview(fileId_t id);
    fileBuf_t read(fileId_t id);
    void write(fileId_t id, fileBuf_t buffer);
    void remove(fileId_t id);

private:
    std::string getPreviewFilePath(fileId_t id) const;

    connection::IConnection* _conn;
    const std::string _indexingPath;
    const std::string _storingPath;
    fileId_t _nFiles;
};

}  /* namesapce fnifi */

#endif  /* FNIFI_COLLECTION_HPP */
