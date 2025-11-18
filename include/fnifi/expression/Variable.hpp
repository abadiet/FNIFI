#ifndef FNIFI_FILE_EXPRESSION_VARIABLE_HPP
#define FNIFI_FILE_EXPRESSION_VARIABLE_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/file/Collection.hpp"
#include "fnifi/expression/DiskBacked.hpp"
#include "fnifi/utils.hpp"
#include <string>


namespace fnifi {
namespace expression {

class Variable : public DiskBacked {
public:
    enum Type {
        CTIME,  /* creation time */
        UNKOWN
    };

    static Type GetType(const std::string& name);
    static void Uncache(const std::filesystem::path& collPath, fileId_t id);

    Variable(const std::string& key, const std::filesystem::path& storingPath,
             const std::vector<file::Collection*>& colls);

private:
    expr_t getValue(const file::File* file, bool noCache = false) override;

    Type _type;
    std::string _name;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_EXPRESSION_VARIABLE_HPP */
