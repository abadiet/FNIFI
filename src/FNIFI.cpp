#include "fnifi/FNIFI.hpp"
#include "fnifi/connection/IConnection.hpp"


using namespace fnifi;

FNIFI::FNIFI(std::vector<file::Collection*>& colls,
             connection::IConnection* storingConn, const char* storingPath)
: _colls(colls), _storingConn(storingConn),
    _storingPath(storingPath)
{
    index();

    /* TODO: load last sorting and filtering algorithms */

    {
        size_t nFiles = 0;
        for (const auto& coll : _colls) {
            nFiles += coll->size();
        }
        _sortFiltFiles.reserve(nFiles);
        for (const auto& coll : _colls) {
            for (const auto& file : *coll) {
                _sortFiltFiles.push_back(&file);
            }
        }
    }

    UNUSED(_storingConn);
}

void FNIFI::defragment() {
    for (auto& coll : _colls) {
         coll->defragment();
    }
}

void FNIFI::index() {
    for (auto& coll : _colls) {
        std::unordered_set<fileId_t> removed;
        std::unordered_set<fileId_t> added;
        std::unordered_set<fileId_t> modified;
        coll->index(removed, added, modified);

        /* TODO: update thanks to added and removed sets */
    }
}

void FNIFI::sort(const char* expr) {
    _sortingAlgo.build(expr, _exprHandler);
    TODO
}

void FNIFI::filter(const char* expr) {
    _filteringAlgo.build(expr, _exprHandler);
    TODO
}

const std::vector<const file::File*>& FNIFI::getFiles() const {
    return _sortFiltFiles;
}

std::vector<const file::File*>::const_iterator FNIFI::begin() const {
    return _sortFiltFiles.begin();
}

std::vector<const file::File*>::const_iterator FNIFI::end() const {
    return _sortFiltFiles.end();
}
