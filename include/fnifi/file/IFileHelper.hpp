#ifndef FNIFI_FILE_IFILEHELPER_HPP
#define FNIFI_FILE_IFILEHELPER_HPP

#include "fnifi/utils.hpp"


namespace fnifi {
namespace file {

class IFileHelper {
public:
    virtual ~IFileHelper();
    virtual std::string getFilePath(fileId_t id) = 0;
    virtual fileBuf_t preview(fileId_t id) = 0;
    virtual fileBuf_t read(fileId_t id) = 0;
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_IFILEHELPER_HPP */
