#ifndef FNIFI_FILE_IFILEHELPER_HPP
#define FNIFI_FILE_IFILEHELPER_HPP

#include "fnifi/utils/utils.hpp"
#include "fnifi/file/Kind.hpp"
#include <sys/stat.h>


namespace fnifi {
namespace file {

class IFileHelper {
public:
    virtual ~IFileHelper();
    virtual std::string getFilePath(fileId_t id) = 0;
    virtual std::string getLocalPreviewFilePath(fileId_t id) = 0;
    virtual std::string getLocalCopyFilePath(fileId_t id) = 0;
    virtual struct stat getStats(fileId_t id) = 0;
    virtual Kind getKind(fileId_t id) = 0;
    virtual fileBuf_t read(fileId_t id, bool nocache = false) = 0;
    virtual std::string getName() const = 0;
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_IFILEHELPER_HPP */
