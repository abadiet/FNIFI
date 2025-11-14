#ifndef FNIFI_FILE_FILE_HPP
#define FNIFI_FILE_FILE_HPP

#include "fnifi/file/Collection.hpp"
#include "fnifi/file/MetadataType.hpp"
#include "fnifi/utils.hpp"
#include <exiv2/exiv2.hpp>
#include <string>
#include <ostream>


namespace fnifi {
namespace file {

class File {
public:
    File(fileId_t id, Collection* coll);
    std::string getPath() const;
    std::ostream& getMetadata(std::ostream& os, MetadataType type,
                              const char* key) const;
    fileBuf_t preview() const;
    fileBuf_t read() const;

private:
    const fileId_t _id;
    Collection* _coll;
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_FILE_HPP */
