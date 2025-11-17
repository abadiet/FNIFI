#ifndef FNIFI_FNIFI_HPP
#define FNIFI_FNIFI_HPP

#include "fnifi/connection/IConnection.hpp"
#include "fnifi/file/Collection.hpp"
#include "fnifi/file/File.hpp"
#include "fnifi/expression/Expression.hpp"
#include <sxeval/SXEval.hpp>
#include <vector>
#include <set>
#include <unordered_set>
#include <iterator>
#include <cstddef>


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

    FNIFI(std::vector<file::Collection*>& colls,
          connection::IConnection* storignConn, const char* storingPath);
    void index();
    void defragment();
    void sort(const char* expr);
    void filter(const char* expr);
    Iterator begin();
    Iterator end();

private:
    std::vector<file::Collection*> _colls;
    expression::Expression _sortExpr;
    expression::Expression _filtExpr;
    fileset_t _files;
    std::unordered_set<const file::File*> _toRemove;
    connection::IConnection* _storingConn;
    std::string _storingPath;
};

}  /* namespace fnifi */

#endif  /* FNIFI_FNIFI_HPP */
