#ifndef FNIFI_EXPRESSION_EXPRESSION_HPP
#define FNIFI_EXPRESSION_EXPRESSION_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/file/Collection.hpp"
#include "fnifi/expression/Variable.hpp"
#include "fnifi/expression/DiskBacked.hpp"
#include "fnifi/utils/SyncDirectory.hpp"
#include "fnifi/utils/utils.hpp"
#include <sxeval/SXEval.hpp>
#include <string>
#include <memory>


namespace fnifi {
namespace expression {

class Expression : public DiskBacked {
public:
    static void Uncache(const utils::SyncDirectory& sync,
                        const std::filesystem::path& collPath, fileId_t id);

    Expression(const std::string& expr,
               const utils::SyncDirectory& storing,
               const std::vector<file::Collection*>& colls);
    void disableSync(const std::string& collName, bool pull = true) override;
    void enableSync(const std::string& collName, bool push = true) override;

private:
    struct RefVar {
        std::unique_ptr<Variable> var;
        expr_t ref;
    };

    expr_t getValue(const file::File* file) override;

    std::function<expr_t&(const std::string&)> _handler;
    sxeval::SXEval<expr_t> _sxeval;
    std::vector<RefVar> _vars;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_EXPRESSION_EXPRESSION_HPP */
