#include "fnifi/FNIFI.hpp"


using namespace fnifi;

FNIFI::FNIFI(std::vector<connection::IConnection*>& conns,
      std::vector<Collection>& colls, const char* storingPath)
: _conns(conns), _colls(colls), _storingPath(storingPath)
{

}

void FNIFI::index() {
    TODO
}

void FNIFI::sort(const char* expr) {
    _sortingAlgo.build(expr, _exprHandler);
    TODO
}

void FNIFI::filter(const char* expr) {
    _filteringAlgo.build(expr, _exprHandler);
    TODO
}

const std::vector<File*>& FNIFI::getFiles() const {
    return _sortFiltFiles;
}

std::vector<File*>::const_iterator FNIFI::begin() const {
    return _sortFiltFiles.begin();
}

std::vector<File*>::const_iterator FNIFI::end() const {
    return _sortFiltFiles.end();
}
