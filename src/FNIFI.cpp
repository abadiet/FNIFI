#include "fnifi/FNIFI.hpp"
#include <ctime>
#include <cstdlib>


using namespace fnifi;

FNIFI::Iterator::Iterator(FNIFI::fileset_t::const_iterator p,
                          std::unordered_set<const file::File*>& toRemove,
                          FNIFI::fileset_t& files)
: _p(p), _toRemove(toRemove), _files(files)
{
    DLOG("FNIFI::Iterator", this, "Instanciation for File " << *p)
}

FNIFI::Iterator::reference FNIFI::Iterator::operator*() const {
    return *_p;
}

FNIFI::Iterator::pointer FNIFI::Iterator::operator->() const {
    return *_p;
}

FNIFI::Iterator& FNIFI::Iterator::operator++() {
    if (*_p == *_files.end()) {
        /* no elements remainings */
        return *this;
    }

    ++_p;
    if (*_p == *_files.end()) {
        /* no elements remainings */
        return *this;
    }

    auto it = _toRemove.find(*_p);
    while (it != _toRemove.end()) {
        /* this iterator should be removed */
        _p = _files.erase(_p);
        if (*_p == *_files.end()) {
            /* no elements remainings */
            return *this;
        }
        _toRemove.erase(it);
        it = _toRemove.find(*_p);
    }

    if ((*_p)->isFilteredOut()) {
        /* this file is filtered out */
        return ++(*this);
    }

    return *this;
}

FNIFI::Iterator FNIFI::Iterator::operator++(int) {
    const auto tmp = *this;
    ++(*this);
    return tmp;
}

bool FNIFI::Iterator::operator==(const Iterator& other) const {
    return _p == other._p;
}

bool FNIFI::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

FNIFI::FNIFI(const utils::SyncDirectory& storing)
: _sortExpr(nullptr), _filtExpr(nullptr),
    _storing(storing)
{
    DLOG("FNIFI", this, "Instanciation with SyncDirectory " << &storing)

    std::srand(static_cast<unsigned int>(std::time({})));
}

void FNIFI::addCollection(file::Collection& coll) {
    indexColl(&coll);

    for (const auto& file : coll) {
        _files.insert(&file.second);
    }

    if (_sortExpr) {
        sortColl(&coll);
    }

    if (_filtExpr) {
        filterColl(&coll);
    }

    _colls.push_back(&coll);
}


void FNIFI::defragment() {
    DLOG("FNIFI", this, "Defragmentation")
 
    for (auto& coll : _colls) {
         coll->defragment();
    }
}

void FNIFI::index() {
    DLOG("FNIFI", this, "Indexation")

    for (auto& coll : _colls) {
        indexColl(coll);
    }
}

void FNIFI::sort(const std::string& expr) {
    DLOG("FNIFI", this, "Sorting with expresion \"" << expr << "\"")

    _files.clear();
    _sortExpr = std::make_unique<expression::Expression>(expr, _storing,
                                                         _colls);
    for (const auto& coll : _colls) {
        sortColl(coll);
    }
}

void FNIFI::filter(const std::string& expr) {
    DLOG("FNIFI", this, "Filtering with expresion \"" << expr << "\"")

    _filtExpr = std::make_unique<expression::Expression>(expr, _storing,
                                                         _colls);
    for (const auto& coll : _colls) {
        filterColl(coll);
    }
}

void FNIFI::clearSort() {
    DLOG("FNIFI", this, "Clearing sorting algorithm")

    _sortExpr = nullptr;
}

void FNIFI::clearFilter() {
    DLOG("FNIFI", this, "Filtering sorting algorithm")

    _filtExpr = nullptr;
}

FNIFI::Iterator FNIFI::begin() {
    return Iterator(_files.begin(), _toRemove, _files);
}

FNIFI::Iterator FNIFI::end() {
    return Iterator(_files.end(), _toRemove, _files);
}

FNIFI::fileset_t FNIFI::getFiles() const {
    return _files;
}

void FNIFI::indexColl(file::Collection* coll) {
    std::unordered_set<std::pair<const file::File*, fileId_t>> removed;
    std::unordered_set<const file::File*> added;
    std::unordered_set<file::File*> modified;

    coll->index(removed, added, modified);

    ILOG("FNIFI", this, "Collection " << &coll << " found "
         << removed.size() << " removed files, " << added.size()
         << " added and " << modified.size() << " modified")

    /* update expressions */
    const auto collHash = utils::Hash(coll->getName());
    for (const auto& file : removed) {
        expression::Expression::Uncache(_storing, collHash, file.second);
        expression::Variable::Uncache(_storing, collHash, file.second);
        _toRemove.insert(file.first);
    }
    for (const auto& file : added) {
        /* process the file before inserting it */
        if (_sortExpr) {
            _sortExpr->get(file);
        }
        if (_filtExpr) {
            _filtExpr->get(file);
        }
        _files.insert(file);
    }
    for (auto& file : modified) {
        /* uncache for every expressions */
        const auto id = file->getId();
        expression::Expression::Uncache(_storing, collHash, id);
        expression::Variable::Uncache(_storing, collHash, id);

        /* overwrite the cache of the current expressions */
        bool hasChanged = false;
        if (_sortExpr) {
            const auto prevScore = file->getSortingScore();
            const auto score = _sortExpr->get(file);
            file->setSortingScore(score);
            hasChanged = hasChanged || (score != prevScore);
        }
        if (_filtExpr) {
            const auto prevCheck = file->isFilteredOut();
            const auto check = _filtExpr->get(file);
            file->setIsFilteredOut(check);
            hasChanged = hasChanged || (check != prevCheck);
        }

        /* update _files */
        if (hasChanged) {
            auto pos = _files.find(file);
            if (pos != _files.end()) {
                _files.erase(pos);
            }
            _files.insert(file);
        }
    }
}

void FNIFI::sortColl(file::Collection* coll) {
    /* disable synchronization during the process to avoid too many calls
         */
    const auto collName = coll->getName();
    _sortExpr->disableSync(collName);

    for (auto& file : *coll) {
        const auto score = _sortExpr->get(&file.second);
        file.second.setSortingScore(score);
        _files.insert(&file.second);
    }

    _sortExpr->enableSync(collName);
}

void FNIFI::filterColl(file::Collection* coll) {
    /* disable synchronization during the process to avoid too many calls
         */
    const auto collName = coll->getName();
    _filtExpr->disableSync(collName);

    for (auto& file : *coll) {
        const auto check = (_filtExpr->get(&file.second) == 0);
        file.second.setIsFilteredOut(check);
    }

    _filtExpr->enableSync(collName);
}
