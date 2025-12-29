#ifndef FNIFI_FILE_EXPRESSION_VARIABLE_HPP
#define FNIFI_FILE_EXPRESSION_VARIABLE_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/file/Collection.hpp"
#include "fnifi/file/Info.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include "fnifi/expression/Kind.hpp"
#include "fnifi/utils/utils.hpp"
#include <string>


namespace fnifi {
namespace expression {

class Variable {
public:
    static Kind GetKind(const std::string& name);

    Variable(const std::string& key,
             const std::vector<file::Collection*>& colls);

    expr_t get(const file::File* file);
    void addCollection(const file::Collection& coll);
    void disableSync(const std::string& collName, bool pull = true);
    void enableSync(const std::string& collName, bool push = true);

private:
    std::unordered_map<std::string, file::Info<expr_t>*> _infos;
    Kind _kind;
    std::string _name;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_EXPRESSION_VARIABLE_HPP */
