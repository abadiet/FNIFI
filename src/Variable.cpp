#include "fnifi/expression/Variable.hpp"

#define VARIABLES_DIRNAME "variables"


using namespace fnifi;
using namespace fnifi::expression;

Variable::Type Variable::GetType(const std::string& name) {
    if (name == "ctime") return Type::CTIME;
    return Type::UNKOWN;
}

void Variable::Uncache(const std::filesystem::path& collPath, fileId_t id) {
    DiskBacked::Uncache(collPath / VARIABLES_DIRNAME, id);
}

Variable::Variable(const std::string& key,
                   const std::filesystem::path& storingPath,
                   const std::vector<file::Collection*>& colls)
: DiskBacked(key, storingPath, colls, VARIABLES_DIRNAME)
{
    /* decompose the key */
    const auto pos = key.find('.');
    if (pos == std::string::npos) {
        /* this is a listed variable */
        _type = Variable::GetType(key);
        _name = key;
    } else {
        /* this is an unkown variable */
        _type = Variable::GetType(key.substr(pos));
        _name = key.substr(pos + 1);
    }
}

expr_t Variable::getValue(const file::File* file, bool noCache) {
    switch (_type) {
        case CTIME:
            {
                const auto time = file->getStats().st_ctimespec;
                return static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec
                                           );
            }
        case UNKOWN:
            {
                TODO
            }
    }

    UNUSED(noCache);
}
