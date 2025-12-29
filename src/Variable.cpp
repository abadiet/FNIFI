#include "fnifi/expression/Variable.hpp"


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

Variable::Variable(const std::string& key,
                   const std::vector<file::Collection*>& colls)
{
    DLOG("Variable", this, "Instanciation for key \"" << key << "\"")

    /* decompose the key */
    const auto pos = key.find('.');
    if (pos == std::string::npos) {
        /* this is a listed variable */
        _kind = Variable::GetKind(key);
        _name = "";
    } else {
        /* this is an unkown variable */
        _kind = Variable::GetKind(key.substr(pos));
        _name = key.substr(pos + 1);
    }

    if (_kind == UNKNOWN) {
        std::ostringstream msg;
        msg << "Invalid key \"" << key << "\"";
        ELOG("Variable", this, msg.str())
        throw std::runtime_error(msg.str());
    }

    for (const auto& coll : colls) {
        _infos.insert(std::make_pair(coll->getName(),
            file::Info<expr_t>::Build(coll, _kind, _name)));
   }
}

expr_t Variable::get(const file::File* file) {
    DLOG("Variable", this, "Getting value for File " << file)

    /* WARNING: the actual file wrapped by the variable may not exists here
     * (but file != nullptr) */

    const auto info = _infos.find(file->getCollectionName());
    if (info == _infos.end()) {
        ELOG("Info", this, "Lock called on a file that belongs to an "
             "unknown Collection (" << file->getCollectionName()
             << ") Aborting the call.")
        return EMPTY_EXPR_T;
    }

    expr_t res;
    if (info->second->get(file, res)) {
        return res;
    }
    return EMPTY_EXPR_T;
}

void Variable::addCollection(const file::Collection& coll) {
    _infos.insert(std::make_pair(coll.getName(),
        file::Info<expr_t>::Build(&coll, _kind, _name)));
}


void Variable::disableSync(const std::string& collName, bool pull) {
    const auto info = _infos.find(collName);
    if (info == _infos.end()) {
        ELOG("Info", this, "Lock called on a file that belongs to an "
             "unknown Collection (" << collName << ") Aborting the call.")
        return;
    }

    info->second->disableSync(pull);
}

void Variable::enableSync(const std::string& collName, bool push) {
    const auto info = _infos.find(collName);
    if (info == _infos.end()) {
        ELOG("Info", this, "Unlock called on a file that belongs to an "
            "unknown Collection (" << collName << ") Aborting the call.")
        return;
    }

    info->second->enableSync(push);
}
