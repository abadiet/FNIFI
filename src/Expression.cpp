#include "fnifi/expression/Expression.hpp"

#define EXPRESSIONS_DIRNAME "expressions"


using namespace fnifi;
using namespace fnifi::expression;

void Expression::Uncache(const utils::SyncDirectory& sync,
                         const std::filesystem::path& collPath, fileId_t id)
{
    DiskBacked::Uncache(sync, collPath / EXPRESSIONS_DIRNAME, id);
}

Expression::Expression(const std::string& expr,
                       const utils::SyncDirectory& storing,
                       const std::vector<file::Collection*>& colls)
: DiskBacked(expr, storing, colls, EXPRESSIONS_DIRNAME)
{
    DLOG("Expression", this, "Instanciation for expr \"" << expr << "\"")

    /* build sxeval */
    _handler =
        [&](const std::string& name) -> expr_t& {
            _vars.push_back({
                std::make_unique<Variable>(name, colls), 0});
            return _vars.back().ref;
        };
    _sxeval.build(expr, _handler);
}

expr_t Expression::getValue(const file::File* file) {
    DLOG("Expression", this, "Getting value for File " << file)

    /* fill the variables */
    for (auto& var : _vars) {
        var.ref = var.var->get(file);
    }

    /* run sxeval */
    return _sxeval.execute();
}

void Expression::addCollection(const file::Collection& coll) {
    DiskBacked::addCollection(coll);

    for (auto& var : _vars) {
        var.var->addCollection(coll);
    }
}

void Expression::disableSync(const std::string& collName, bool pull) {
    DiskBacked::disableSync(collName, pull);

    for (auto& var : _vars) {
        var.var->disableSync(collName, pull);
    }
}

void Expression::enableSync(const std::string& collName, bool push) {
    DiskBacked::enableSync(collName, push);

    for (auto& var : _vars) {
        var.var->enableSync(collName, push);
    }
}
