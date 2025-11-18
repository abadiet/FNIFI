#ifndef FNIFI_EXPRESSION_DISKBACKED_HPP
#define FNIFI_EXPRESSION_DISKBACKED_HPP

#include "fnifi/file/Collection.hpp"
#include "fnifi/utils.hpp"
#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>


namespace fnifi {
namespace expression {

class DiskBacked {
public:
    static void Uncache(const std::filesystem::path& path, fileId_t id);

    DiskBacked(const std::string& key,
               const std::filesystem::path& storingPath,
               const std::vector<file::Collection*>& colls,
               const std::filesystem::path& parentDirName);
    virtual ~DiskBacked();
    expr_t get(const file::File* file, bool noCache = false);
    void update();

private:
    struct StoredColl {
        std::unique_ptr<std::fstream> file;
        fileId_t maxId;
        std::filesystem::path path;
    };

    virtual expr_t getValue(const file::File* file, bool noCache = false) = 0;

    std::unordered_map<std::string, StoredColl> _storedColls;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_DISKBACKED_HPP */
