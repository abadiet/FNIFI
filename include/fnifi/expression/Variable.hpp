#ifndef FNIFI_FILE_EXPRESSION_VARIABLE_HPP
#define FNIFI_FILE_EXPRESSION_VARIABLE_HPP

#include "fnifi/file/File.hpp"
#include "fnifi/file/Collection.hpp"
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

    static void Init(const std::filesystem::path& storingPath,
               const std::vector<file::Collection*>& colls);
    static Variable& Build(const std::string& key);
    static Type GetType(const std::string& name);

    ~Variable();
    expr_t get(const file::File* file, bool noCache = false);

    /**
     * PRIVATE USE ONLY
     *
     * TODO
     */
    Variable(const std::string& key, Type type, const std::string& name);

private:
    struct StoredColl {
        std::fstream file;
        fileId_t maxId;
        std::filesystem::path path;
    };

    expr_t getValue(const file::File* file);

    static std::unordered_map<std::string, Variable> _built;
    static std::unordered_map<std::string, StoredColl> _storedColls;
    const Type _type;
    const std::string _key;
};

}  /* namespace expression */
}  /* namespace fnifi */

#endif  /* FNIFI_FILE_EXPRESSION_VARIABLE_HPP */
