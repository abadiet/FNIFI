#include "fnifi/expression/Variable.hpp"

#define VARIABLES_DIRNAME "variables"
#define RESULTS_FILENAME "results.fnifi"


using namespace fnifi;
using namespace fnifi::expression;

std::unordered_map<std::string, Variable> Variable::_built;
std::unordered_map<std::string, Variable::StoredColl> Variable::_storedColls;

void Variable::Init(const std::filesystem::path& storingPath,
               const std::vector<file::Collection*>& colls)
{
    for (const auto& coll : colls) {
        /* create the folders if needed */
        const auto name = coll->getName();
        const auto dir = storingPath / Hash(name);
        std::filesystem::create_directories(dir);

        /* fill _stored */
        _storedColls.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(std::fstream(), 0, dir)
        );
    }
}

Variable& Variable::Build(const std::string& key) {
    if (_storedColls.size() == 0) {
        std::string msg("Variable::Build has been called before its "
             "initialisation. Please run Variable::Init before any "
             "Variable::Build.");
        ELOG(msg)
        throw std::runtime_error(msg);
    }

    const auto it = _built.find(key);
    if (it != _built.end()) {
        /* the key has already been build */
        return it->second;
    }

    /* decompose the key */
    const auto pos = key.find('.');
    Type type;
    std::string name;
    if (pos == std::string::npos) {
        /* this is a listed variable */
        type = Variable::GetType(key);
        name = key;
    } else {
        /* this is an unkown variable */
        type = Variable::GetType(key.substr(pos));
        name = key.substr(pos + 1);
    }

    /* IDK why this works */
    auto result = _built.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(key, type, name)
    );

    return result.first->second;
}

Variable::Type Variable::GetType(const std::string& name) {
    if (name == "ctime") return Type::CTIME;
    return Type::UNKOWN;
}

Variable::~Variable() {
    for (auto& stored : _storedColls) {
        if (stored.second.file.is_open()) {
            stored.second.file.close();
        }
    }
}

expr_t Variable::get(const file::File* file, bool noCache) {
    /* get the associated stored file */
    const auto stored = _storedColls.find(file->getCollectionName());
    if (stored == _storedColls.end()) {
        ELOG("Expression " << this << " has been called on a file that belongs"
             " to an unknown Collection (" << file->getCollectionName()
             << ") Aborting the call.")
        return 0;
    }

    const auto id = file->getId();
    const auto pos = id * sizeof(expr_t);

    if (id <= stored->second.maxId) {
        if (!noCache) {
            /* the value may be saved */
            expr_t res;
            stored->second.file.seekg(std::streamoff(pos));
            Deserialize(stored->second.file, res);

            if (res != EMPTY_EXPR_T) {
                /* the value was saved */
                return res;
            }
        }
    } else {
        /* filling the file up to the position of the value */
        stored->second.file.seekp(0, std::ios::end);
        for (auto i = stored->second.maxId; i < id; ++i) {
            /* TODO avoid multiples std::ofstream::write calls */
            Serialize(stored->second.file, EMPTY_EXPR_T);
        }
        stored->second.maxId = id;
    }

    const auto res = getValue(file);
    stored->second.file.seekp(std::streamoff(pos));
    Serialize(stored->second.file, res);

    return res;
}

Variable::Variable(const std::string& key, Variable::Type type,
                   const std::string& name)
: _type(type), _key(key)
{
    /* update _stored */
    for (auto& s : _storedColls) {
        /* close previous results.fnifi file if needed */
        if (s.second.file.is_open()) {
            s.second.file.close();
            s.second.maxId = 0;
        }

        /* create directory if needed */
        const auto dir = s.second.path / VARIABLES_DIRNAME / Hash(name);
        std::filesystem::create_directories(dir);

        /* create or open the results.fnifi file */
        const auto filename = dir / RESULTS_FILENAME;
        if (std::filesystem::exists(filename)) {
            s.second.file =  std::fstream(filename, std::ios::in |
                                          std::ios::out | std::ios::binary |
                                          std::ios::ate);
            s.second.maxId = static_cast<fileId_t>(static_cast<size_t>(
                s.second.file.tellg()) / sizeof(expr_t));
        } else {
            s.second.file =  std::fstream(filename, std::ios::in |
                                          std::ios::out | std::ios::binary |
                                          std::ios::ate | std::ios::trunc);
            s.second.maxId = 0;
        }
    }
}

expr_t Variable::getValue(const file::File* file) {
    switch (_type) {
        case CTIME:
            {
                const auto time = file->getStats().st_ctimespec;
                return static_cast<expr_t>(S_TO_NS(time.tv_sec) + time.tv_nsec
                                           );
            }
        case UNKOWN:
            {
                /* TODO */
                return 0;
            }
    }
}
