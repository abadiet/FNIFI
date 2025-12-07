#ifndef FNIFI_FILE_COLLECTION_HPP
#define FNIFI_FILE_COLLECTION_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include "fnifi/file/Kind.hpp"
#include "fnifi/file/IFileHelper.hpp"
#include "fnifi/file/File.hpp"
#include "fnifi/utils/utils.hpp"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <time.h>
#include <filesystem>
#include <memory>


namespace fnifi {
namespace file {

class Collection : virtual public IFileHelper {
public:
    Collection(connection::IConnection* indexingConn,
               utils::SyncDirectory& storing);
    Collection(Collection&& other) noexcept;
    ~Collection() override;
    void index(
        std::unordered_set<std::pair<const file::File*, fileId_t>>& removed,
        std::unordered_set<const file::File*>& added,
        std::unordered_set<file::File*>& modified);
    void defragment();
    std::string getFilePath(fileId_t id) override;
    struct stat getStats(fileId_t id) override;
    Kind getKind(fileId_t id) override;
    fileBuf_t preview(fileId_t id) override;
    fileBuf_t read(fileId_t id) override;
    std::string getName() const override;
    std::unordered_map<fileId_t, File>::const_iterator begin() const;
    std::unordered_map<fileId_t, File>::const_iterator end() const;
    std::unordered_map<fileId_t, File>::iterator begin();
    std::unordered_map<fileId_t, File>::iterator end();
    size_t size() const;

private:
    using offset_t = size_t;
    using lenght_t = unsigned short;
    struct __attribute__((packed)) MapNode {
        offset_t offset;
        lenght_t lenght;
    };
    struct __attribute__((packed)) Info {
        struct timespec lastIndexing = {0, 0};
    };

    static Kind GetKind(const fileBuf_t& buf);

    std::unordered_map<fileId_t, File> _files;
    connection::IConnection* _indexingConn;
    const utils::SyncDirectory& _storing;
    const std::filesystem::path _storingPath;
    std::unique_ptr<utils::SyncDirectory::FileStream> _mapping;
    std::unique_ptr<utils::SyncDirectory::FileStream> _filepaths;
    std::unique_ptr<utils::SyncDirectory::FileStream> _info;
    std::unordered_set<fileId_t> _availableIds;
    bool _previewsMkdir;
};

}  /* namespace file */
}  /* namesapce fnifi */

#endif  /* FNIFI_FILE_COLLECTION_HPP */
