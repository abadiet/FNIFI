#ifndef FNIFI_COLLECTION_HPP
#define FNIFI_COLLECTION_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils.hpp"
#include <string>


namespace fnifi {

class Collection {
public:
    Collection(const connection::IConnection* conn, const char* indexingPath,
               const char* storingPath);
    fileId_t getFirstId() const;
    fileId_t getLastId() const;
    std::string getFilePath(fileId_t id) const;
    fileBuf_t read(fileId_t id) const;
    void write(fileId_t id, fileBuf_t buffer) const;
    void remove(fileId_t id) const;

private:
    const connection::IConnection* _conn;
    const std::string _indexingPath;
    const std::string _storingPath;
};

}  /* namesapce fnifi */

#endif  /* FNIFI_COLLECTION_HPP */
