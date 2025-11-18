#include "fnifi/expression/Variable.hpp"
#include <limits>


using namespace fnifi;
using namespace fnifi::expression;

std::unordered_map<std::string, Variable> Variable::_built;

Variable& Variable::Build(Variable::Type type, const std::string& key,
                          const std::filesystem::path& storingPath)
{
    const auto it = _built.find(key);
    if (it != _built.end()) {
        /* the key has already been build */
        return it->second;
    }

    /* IDK why this works */
    auto result = _built.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(type, key, storingPath)
    );

    return result.first->second;
}

Variable::Type Variable::GetType(const std::string& name) {
    if (name == "ctime") return Type::CTIME;
    return Type::UNKOWN;
}

Variable::~Variable() {
    if (_stored.is_open()) {
        _stored.close();
    }
}

expr_t Variable::get(const file::File* file) {
    const auto id = file->getId();
    const auto pos = id * sizeof(expr_t);

    if (id <= _maxId) {
        /* the value may be saved */
        expr_t res;
        _stored.seekg(std::streamoff(pos));
        Deserialize(_stored, res);

        if (res != EMPTY_EXPR_T) {
            /* the value was saved */
            return res;
        }
    } else {
        /* filling the file up to the position of the value */
        _stored.seekp(0, std::ios::end);
        for (auto i = _maxId; i < id; ++i) {
            /* TODO avoid multiples std::ofstream::write calls */
            Serialize(_stored, EMPTY_EXPR_T);
        }
        _maxId = id;
    }

    const auto res = getValue(file);
    _stored.seekp(std::streamoff(pos));
    Serialize(_stored, res);

    return res;
}

Variable::Variable(Variable::Type type, const std::string& key,
                   const std::filesystem::path& storingPath)
: _maxId(0), _type(type), _key(key)
{
    const auto filename = storingPath / (Hash(_key) + ".fnifi");
    if (std::filesystem::exists(filename)) {
        _stored = std::fstream(filename, std::ios::in | std::ios::out |
                               std::ios::binary | std::ios::ate);
        _maxId = static_cast<fileId_t>(static_cast<size_t>(_stored.tellg())
                                       / sizeof(expr_t));
    } else {
        _stored = std::fstream(filename, std::ios::in | std::ios::out |
                               std::ios::binary | std::ios::trunc);
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
