#ifndef FNIFI_FILE_AFILEEHELPER_HPP
#define FNIFI_FILE_AFILEEHELPER_HPP

#include "fnifi/file/InfoType.hpp"
#include "fnifi/utils/utils.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include <sys/stat.h>


namespace fnifi {
namespace file {

template<InfoType T>
class Info;

class AFileHelper {
public:
    AFileHelper(const utils::SyncDirectory& storing,
                const std::filesystem::path& storingPath);

    virtual ~AFileHelper();
    virtual std::string getFilePath(fileId_t id) = 0;
    virtual std::string getLocalPreviewFilePath(fileId_t id) = 0;
    virtual std::string getLocalCopyFilePath(fileId_t id) = 0;
    virtual struct stat getStats(fileId_t id) = 0;
    virtual fileBuf_t read(fileId_t id, bool nocache = false) = 0;
    virtual std::string getName() const = 0;

protected:
    const utils::SyncDirectory& _storing;
    const std::filesystem::path _storingPath;

    template<InfoType T>
    friend class fnifi::file::Info;
};

}  /* namespace file */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_AFILEEHELPER_HPP */
