#ifndef FNIFI_FILE_FILE_HPP
#define FNIFI_FILE_FILE_HPP

#include "fnifi/file/IFileHelper.hpp"
#include "fnifi/file/MetadataType.hpp"
#include "fnifi/utils.hpp"
#include <sys/stat.h>
#include <exiv2/exiv2.hpp>
#include <string>
#include <ostream>


namespace fnifi {
namespace file {

class File {
public:
    struct pCompare {
        bool operator()(const File* a, const File* b) const;
    };

    File(fileId_t id, IFileHelper* helper);
    bool operator==(const File& other) const;
    fileId_t getId() const;
    std::string getPath() const;
    struct stat getStats() const;
    std::ostream& getMetadata(std::ostream& os, MetadataType type,
                              const std::string& key) const;
    fileBuf_t preview() const;
    fileBuf_t read() const;
    void setSortingScore(expr_t score);
    expr_t getSortingScore() const;
    void setIsFilteredOut(bool isFilteredOut);
    bool isFilteredOut() const;
    std::string getCollectionName() const;

private:
    const fileId_t _id;
    expr_t _sortScore;
    bool _filteredOut;
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

template<>
struct hash<std::pair<const fnifi::file::File*, fnifi::fileId_t>> {
    size_t operator()(const std::pair<const fnifi::file::File*,
                      fnifi::fileId_t>& obj) const
    {
        return std::hash<fnifi::fileId_t>()(obj.second);
    }
};

}  /* namespace std */

#endif  /* FNIFI_FILE_FILE_HPP */
