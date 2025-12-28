#include "fnifi/expression/Variable.hpp"

#define VARIABLES_DIRNAME "variables"


using namespace fnifi;
using namespace fnifi::expression;

Kind Variable::GetKind(const std::string& name) {
    if (name == "kind") return Kind::KIND;
    if (name == "size") return Kind::SIZE;
    if (name == "ctime") return Kind::CTIME;
    if (name == "mtime") return Kind::MTIME;
    if (name == "width") return Kind::WIDTH;
    if (name == "height") return Kind::HEIGHT;
    if (name == "duration") return Kind::DURATION;
    if (name == "latitude") return Kind::LATITUDE;
    if (name == "longitude") return Kind::LONGITUDE;
    if (name == "altitude") return Kind::ALTITUDE;
    return Kind::UNKNOWN;
}

void Variable::Uncache(const utils::SyncDirectory& sync,
                       const std::filesystem::path& collPath, fileId_t id)
{
    DiskBacked::Uncache(sync, collPath / VARIABLES_DIRNAME, id);
}

Variable::Variable(const std::string& key,
                   const utils::SyncDirectory& storing,
                   const std::vector<file::Collection*>& colls)
: DiskBacked(key, storing, colls, VARIABLES_DIRNAME)
{
    DLOG("Variable", this, "Instanciation for key \"" << key << "\"")

    /* decompose the key */
    const auto pos = key.find('.');
    if (pos == std::string::npos) {
        /* this is a listed variable */
        _type = Variable::GetKind(key);
        _name = key;
    } else {
        /* this is an unkown variable */
        _type = Variable::GetKind(key.substr(pos));
        _name = key.substr(pos + 1);
    }

    if (_type == UNKNOWN) {
        std::ostringstream msg;
        msg << "Invalid key \"" << key << "\"";
        ELOG("Variable", this, msg.str())
        throw std::runtime_error(msg.str());
    }
}

expr_t Variable::getValue(const file::File* file) {
    DLOG("Variable", this, "Getting value for File " << file)

    /* WARNING: the actual file wrapped by the variable may not exists here
     * (but file != nullptr) */

    expr_t val;
    if (file->get(val, _type, _name)) {
        return val;
    }
    return EMPTY_EXPR_T;
}
