#ifndef FNIFI_FNIFI_HPP
#define FNIFI_FNIFI_HPP

#include "fnifi/file/Collection.hpp"
#include "fnifi/file/File.hpp"
#include "fnifi/expression/Expression.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include <sxeval/SXEval.hpp>
#include <vector>
#include <set>
#include <unordered_set>
#include <iterator>
#include <cstddef>
#include <memory>


namespace fnifi {

class FNIFI {
public:
    typedef std::multiset<const file::File*, file::File::pCompare> fileset_t;
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const file::File*;
        using pointer = const file::File*;
        using reference = const file::File*;

        Iterator(fileset_t::const_iterator p,
                 std::unordered_set<const file::File*>& toRemove,
                 FNIFI::fileset_t& files);
        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        Iterator operator++(int);
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

    private:
        fileset_t::const_iterator _p;
        std::unordered_set<const file::File*>& _toRemove;
        fileset_t& _files;
    };

    FNIFI(const utils::SyncDirectory& storing);
    void addCollection(file::Collection& coll);
    void index();
    void defragment();
    void sort(const std::string& expr);
    void filter(const std::string& expr);
    void clearSort();
    void clearFilter();
    Iterator begin();
    Iterator end();

private:
    void indexColl(file::Collection* coll);
    void sortColl(file::Collection* coll);
    void filterColl(file::Collection* coll);
    std::vector<file::Collection*> _colls;
    std::unique_ptr<expression::Expression> _sortExpr;
    std::unique_ptr<expression::Expression> _filtExpr;
    fileset_t _files;
    std::unordered_set<const file::File*> _toRemove;
    const utils::SyncDirectory& _storing;
};

}  /* namespace fnifi */

#endif  /* FNIFI_FNIFI_HPP */
