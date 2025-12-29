#ifndef FNIFI_FILE_FILE_HPP
#define FNIFI_FILE_FILE_HPP

#include "fnifi/file/AFileHelper.hpp"
#include "fnifi/file/InfoType.hpp"
#include "fnifi/expression/Kind.hpp"
#include "fnifi/file/Kind.hpp"
#include "fnifi/utils/utils.hpp"
#include <sys/stat.h>
#include <string>


namespace fnifi {
namespace file {

class File {
public:
    struct pCompare {
        bool operator()(const File* a, const File* b) const;
    };

    static Kind GetKind(const fileBuf_t& buf);

    File(fileId_t id, AFileHelper* helper);
    bool operator==(const File& other) const;
    fileId_t getId() const;
    std::string getPath() const;
    std::string getLocalPreviewPath() const;
    std::string getLocalCopyPath() const;
    struct stat getStats() const;
    Kind getKind() const;
    template<InfoType T>
    bool get(T& result, expression::Kind kind, const std::string& key = "")
        const;
    fileBuf_t read(bool nocache = false) const;
    void setSortingScore(expr_t score);
    expr_t getSortingScore() const;
    void setIsFilteredOut(bool isFilteredOut);
    bool isFilteredOut() const;
    std::string getCollectionName() const;
    void setHelper(AFileHelper* helper);

private:
    static bool StartWith(const fileBuf_t& buf, const char* chars, size_t n,
                          fileBuf_t::iterator::difference_type offset = 0);

    const fileId_t _id;
    expr_t _sortScore;
    bool _filteredOut;
    AFileHelper* _helper;
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
