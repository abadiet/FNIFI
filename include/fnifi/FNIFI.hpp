#ifndef FNIFI_FNIFI_HPP
#define FNIFI_FNIFI_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/file/Collection.hpp"
#include "fnifi/file/File.hpp"
#include "fnifi/utils.hpp"
#include <sxeval/SXEval.hpp>
#include <vector>


namespace fnifi {

class FNIFI {
public:
    FNIFI(std::vector<connection::IConnection*>& conns,
          std::vector<file::Collection*>& colls,
          connection::IConnection* storingConn, const char* storingPath);
    void index();
    void sort [[noreturn]] (const char* expr);
    void filter [[noreturn]] (const char* expr);
    const std::vector<file::File*>& getFiles() const;
    std::vector<file::File*>::const_iterator begin() const;
    std::vector<file::File*>::const_iterator end() const;

private:
    std::vector<connection::IConnection*> _conns;
    std::vector<file::Collection*> _colls;
    std::vector<file::File> _files;
    sxeval::SXEval<expr_t> _sortingAlgo;
    sxeval::SXEval<expr_t> _filteringAlgo;
    std::function<expr_t&(const std::string&)> _exprHandler;
    std::vector<file::File*> _sortedFiles;
    std::vector<file::File*> _filteredFiles;
    std::vector<file::File*> _sortFiltFiles;
    connection::IConnection* _storingConn;
    std::string _storingPath;
};

}  /* namespace fnifi */

#endif  /* FNIFI_FNIFI_HPP */
