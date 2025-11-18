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

FNIFI::FNIFI(const std::vector<file::Collection*>& colls,
             connection::IConnection* storingConn,
             const std::filesystem::path& storingPath)
: _colls(colls), _sortExpr(nullptr), _filtExpr(nullptr),
    _storingConn(storingConn), _storingPath(storingPath)
{
    /* create the folder if needed */
    std::filesystem::create_directories(_storingPath);

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
        std::unordered_set<file::File*> modified;

        coll->index(removed, added, modified);

        /* update expressions */
        const auto collHash = Hash(coll->getName());
        for (const auto& file : removed) {
            expression::Expression::Uncache(_storingPath / collHash,
                                            file.second);
            expression::Variable::Uncache(_storingPath / collHash,
                                          file.second);
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
            expression::Expression::Uncache(_storingPath / collHash, id);
            expression::Variable::Uncache(_storingPath / collHash, id);

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
}

void FNIFI::sort(const std::string& expr) {
    _files.clear();
    _sortExpr = std::make_unique<expression::Expression>(expr, _storingPath, _colls);
    for (const auto& coll : _colls) {
        for (auto& file : *coll) {
            const auto score = _sortExpr->get(&file.second);
            file.second.setSortingScore(score);
            _files.insert(&file.second);
        }
    }
}

void FNIFI::filter(const std::string& expr) {
    _filtExpr = std::make_unique<expression::Expression>(expr, _storingPath, _colls);
    for (const auto& coll : _colls) {
        for (auto& file : *coll) {
            const auto check = (_filtExpr->get(&file.second) == 0);
            file.second.setIsFilteredOut(check);
        }
    }
}

void FNIFI::clearSort() {
    _sortExpr = nullptr;
}

void FNIFI::clearFilter() {
    _filtExpr = nullptr;
}

FNIFI::Iterator FNIFI::begin() {
    return Iterator(_files.begin(), _toRemove, _files);
}

FNIFI::Iterator FNIFI::end() {
    return Iterator(_files.end(), _toRemove, _files);
}
