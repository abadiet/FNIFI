#ifndef FNIFI_EXPRESSION_EXPRESSION_HPP
#define FNIFI_EXPRESSION_EXPRESSION_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/file/Collection.hpp"
#include "fnifi/expression/Variable.hpp"
#include "fnifi/utils.hpp"
#include <sxeval/SXEval.hpp>
#include <string>
#include <unordered_map>


namespace fnifi {
namespace expression {

class Expression {
public:
    Expression(const std::filesystem::path& storingPath,
               const std::vector<file::Collection*>& colls);
    ~Expression();
    void build(const std::string& expr);
    expr_t run(const file::File* file, bool noCache = false);
    void remove(fileId_t id, const std::string& collectionName);

private:
    struct RefVar {
        Variable* var;
        expr_t ref;
    };
    struct StoredColl {
        std::fstream file;
        fileId_t maxId;
        std::filesystem::path path;
    };

    expr_t getValue(const file::File* file, bool noCache = false);

    const std::filesystem::path _storingPath;
    std::function<expr_t&(const std::string&)> _handler;
    sxeval::SXEval<expr_t> _sxeval;
    std::vector<RefVar> _vars;
    std::unordered_map<std::string, StoredColl> _storedColls;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_EXPRESSION_HPP */
