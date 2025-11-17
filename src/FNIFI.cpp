#include "fnifi/FNIFI.hpp"
#include "fnifi/connection/IConnection.hpp"


using namespace fnifi;

FNIFI::Iterator::Iterator(FNIFI::fileset_t::const_iterator p,
                          std::unordered_set<const file::File*>& toRemove,
                          FNIFI::fileset_t& files)
: _p(p), _toRemove(toRemove), _files(files)
{}

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

FNIFI::FNIFI(std::vector<file::Collection*>& colls,
             connection::IConnection* storingConn, const char* storingPath)
: _colls(colls), _storingConn(storingConn),
    _storingPath(storingPath)
{
    index();

    for (const auto& coll : _colls) {
        for (const auto& file : *coll) {
            _files.insert(&file.second);
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
        std::unordered_set<std::pair<const file::File*, fileId_t>> removed;
        std::unordered_set<const file::File*> added;
        std::unordered_set<const file::File*> modified;

        coll->index(removed, added, modified);

        /* update expressions */
        /* TODO: update saved expressions as well */
        for (const auto& file : removed) {
            _sortExpr.remove(file.second);
            _filtExpr.remove(file.second);
            _toRemove.insert(file.first);
        }
        for (const auto& file : added) {
            _sortExpr.run(file);
            _filtExpr.run(file);
            _files.insert(file);
        }
        for (const auto& file : modified) {
            _sortExpr.run(file, true);
            _filtExpr.run(file, true);
            _files.insert(file);
        }
    }
}

void FNIFI::sort(const char* expr) {
    _files.clear();
    _sortExpr.build(expr, _storingPath);
    for (const auto& coll : _colls) {
        for (auto& file : *coll) {
            const auto score = _sortExpr.run(&file.second);
            file.second.setSortingScore(score);
            _files.insert(&file.second);
        }
    }
}

void FNIFI::filter(const char* expr) {
    _filtExpr.build(expr, _storingPath);
    for (const auto& coll : _colls) {
        for (auto& file : *coll) {
            const auto check = (_filtExpr.run(&file.second) == 0);
            file.second.setIsFilteredOut(check);
        }
    }
}

FNIFI::Iterator FNIFI::begin() {
    return Iterator(_files.begin(), _toRemove, _files);
}

FNIFI::Iterator FNIFI::end() {
    return Iterator(_files.end(), _toRemove, _files);
}
