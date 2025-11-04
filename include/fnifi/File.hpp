#ifndef FNIFI_FILE_HPP
#define FNIFI_FILE_HPP

#include "fnifi/Collection.hpp"
#include "fnifi/utils.hpp"
#include <string>


namespace fnifi {

class File {
public:
    File(fileId_t id, const Collection* coll);
    std::string getPath() const;
    std::string getMetadata(const char* key) const;
    fileBuf_t getPreview() const;
    fileBuf_t getOriginal() const;
    void remove() const;

private:
    const fileId_t _id;
    const Collection* coll;

};

}  /* namespace fnifi */

#endif  /* FNIFI_FILE_HPP */
