#ifndef FNIFI_FILE_HPP
#define FNIFI_FILE_HPP

#include "fnifi/Collection.hpp"
#include "fnifi/utils.hpp"
#include <string>


namespace fnifi {

class File {
public:
    File(fileId_t id, Collection* coll);
    std::string getPath() const;
    std::string getMetadata(const char* key);
    fileBuf_t preview() const;
    fileBuf_t read() const;
    void write(const fileBuf_t& buf);
    void remove() const;

private:
    const fileId_t _id;
    Collection* _coll;
};

}  /* namespace fnifi */

#endif  /* FNIFI_FILE_HPP */
