#ifndef FNIFI_FILE_FILE_HPP
#define FNIFI_FILE_FILE_HPP

#include "fnifi/file/IFileHelper.hpp"
#include "fnifi/file/MetadataType.hpp"
#include "fnifi/utils.hpp"
#include <exiv2/exiv2.hpp>
#include <string>
#include <ostream>


namespace fnifi {
namespace file {

class File {
public:
    File(fileId_t id, IFileHelper* helper);
    bool operator==(const File& other) const;
    fileId_t getId() const;
    std::string getPath() const;
    std::ostream& getMetadata(std::ostream& os, MetadataType type,
                              const char* key) const;
    fileBuf_t preview() const;
    fileBuf_t read() const;

private:
    const fileId_t _id;
    IFileHelper* _helper;
};

}  /* namespace file */
}  /* namespace fnifi */

namespace std {

template<>
struct hash<fnifi::file::File> {
    size_t operator()(const fnifi::file::File& obj) const {
        return std::hash<fnifi::fileId_t>()(obj.getId());
    }
};

}  /* namespace std */

#endif  /* FNIFI_FILE_FILE_HPP */
