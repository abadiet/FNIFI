#ifndef FNIFI_EXPRESSION_DISKBACKED_HPP
#define FNIFI_EXPRESSION_DISKBACKED_HPP

#include "fnifi/file/Collection.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include "fnifi/utils/utils.hpp"
#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>


namespace fnifi {
namespace expression {

class DiskBacked {
public:
    static void Uncache(const utils::SyncDirectory& storing,
                        const std::filesystem::path& path, fileId_t id);

    DiskBacked(const std::string& key,
               const utils::SyncDirectory& storing,
               const std::vector<file::Collection*>& colls,
               const std::filesystem::path& parentDirName);
    virtual ~DiskBacked();
    expr_t get(const file::File* file);
    void addCollection(const file::Collection& coll);
    virtual void disableSync(const std::string& collName, bool pull = true);
    virtual void enableSync(const std::string& collName, bool push = true);

private:
    struct StoredColl {
        std::unique_ptr<utils::SyncDirectory::FileStream> file;
        fileId_t NIds;
    };

    virtual expr_t getValue(const file::File* file) = 0;

    std::unordered_map<std::string, StoredColl> _storedColls;
    const std::string _keyHash;
    const utils::SyncDirectory& _storing;
    const std::filesystem::path& _parentDirName;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_DISKBACKED_HPP */
