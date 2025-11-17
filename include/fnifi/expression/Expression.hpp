#ifndef FNIFI_EXPRESSION_EXPRESSION_HPP
#define FNIFI_EXPRESSION_EXPRESSION_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/expression/Variable.hpp"
#include "fnifi/utils.hpp"
#include <sxeval/SXEval.hpp>
#include <string>


namespace fnifi {
namespace expression {

class Expression {
public:
    Expression();
    void build(const std::string& expr, const std::string& storingPath);
    expr_t run(const file::File* file, bool noCache = false);
    void remove(fileId_t id);

private:
    struct RefVar {
        Variable* var;
        expr_t ref;
    };

    expr_t getValue(const file::File* file);
    static std::string GetHash(const std::string& s);

    std::fstream _stored;
    std::string _storingPath;
    std::function<expr_t&(const std::string&)> _handler;
    sxeval::SXEval<expr_t> _sxeval;
    std::vector<RefVar> _vars;
    fileId_t _maxId;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_EXPRESSION_HPP */
