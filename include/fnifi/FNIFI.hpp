#ifndef FNIFI_FNIFI_HPP
#define FNIFI_FNIFI_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/Collection.hpp"
#include "fnifi/File.hpp"
#include <sxeval/SXEval.hpp>
#include <vector>


namespace fnifi {

class FNIFI {
public:
    FNIFI(std::vector<connection::IConnection>& conns,
          std::vector<Collection>& colls, const char* storingPath);
    void index();
    void sort(const char* expr);
    void filter(const char* expr);
    const std::vector<File*>& getFiles() const;
    const File* begin() const;
    const File* end() const;

private:
    std::vector<connection::IConnection> _conns;
    std::vector<Collection> _colls;
    std::vector<File> _files;
    sxeval::SXEval<expr_t> sortingAlgo;
    sxeval::SXEval<expr_t> filteringAlgo;
    std::vector<File*> _sortedFiles;
    std::vector<File*> _filteredFiles;
    std::vector<File*> _sortFiltFiles;
    std::string _storingPath;
};

}  /* namespace fnifi */

#endif  /* FNIFI_FNIFI_HPP */
