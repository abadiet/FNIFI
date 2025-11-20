#include "fnifi/expression/Variable.hpp"

#define VARIABLES_DIRNAME "variables"


using namespace fnifi;
using namespace fnifi::expression;

Type Variable::GetType(const std::string& name) {
    if (name == "ctime") return Type::CTIME;
    return Type::UNKOWN;
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
        _type = Variable::GetType(key);
        _name = key;
    } else {
        /* this is an unkown variable */
        _type = Variable::GetType(key.substr(pos));
        _name = key.substr(pos + 1);
    }

    if (_type == UNKOWN) {
        std::ostringstream msg;
        msg << "Invalid key \"" << key << "\"";
        ELOG("Variable", this, msg.str())
        throw std::runtime_error(msg.str());
    }
}

expr_t Variable::getValue(const file::File* file) {
    DLOG("Variable", this, "Getting value for File " << file)

    /* TODO consider using File::getMetadata */
    switch (_type) {
        case CTIME:
            {
                const auto time = file->getStats().st_ctimespec;
                return static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec
                                           );
            }
        case UNKOWN:
            throw std::runtime_error("Bad Metadata's type");
        case EXIF:
        case XMP:
        case IPTC:
            TODO
    }
}
