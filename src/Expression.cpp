#include "fnifi/expression/Expression.hpp"

#define EXPRESSIONS_DIRNAME "expressions"


using namespace fnifi;
using namespace fnifi::expression;

Expression::Expression(const std::string& expr,
                       const std::filesystem::path& storingPath,
                       const std::vector<file::Collection*>& colls)
: DiskBacked(expr, storingPath, colls, EXPRESSIONS_DIRNAME)
{
    /* build sxeval */
    _handler =
        [&](const std::string& name) -> expr_t& {
            _vars.push_back({
                std::make_unique<Variable>(name, storingPath, colls), 0});
            return _vars.back().ref;
        };
    _sxeval.build(expr, _handler);
}

expr_t Expression::getValue(const file::File* file, bool noCache) {
    /* fill the variables */
    for (auto& var : _vars) {
        var.ref = var.var->get(file, noCache);
    }

    /* run sxeval */
    return _sxeval.execute();
}
