#ifndef FNIFI_FILE_EXPRESSION_VARIABLE_HPP
#define FNIFI_FILE_EXPRESSION_VARIABLE_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/utils.hpp"
#include <fstream>
#include <string>
#include <unordered_map>


namespace fnifi {
namespace expression {

class Variable {
public:
    enum Type {
        CTIME,  /* creation time */
        UNKOWN
    };

    static Variable& Build(Type type, const std::string& key,
                           const char* storingPath);
    static Type GetType(const std::string& name);

    ~Variable();
    expr_t get(const file::File* file);

    /**
     * PRIVATE USE ONLY
     *
     * TODO
     */
    Variable(Type type, const std::string& key,
                       const char* storingPath);

private:
    expr_t getValue(const file::File* file);

    static std::unordered_map<std::string, Variable> _built;
    std::fstream _stored;
    fileId_t _maxId;
    const Type _type;
    const std::string _key;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_EXPRESSION_VARIABLE_HPP */
