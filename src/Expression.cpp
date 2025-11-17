#include "fnifi/expression/Expression.hpp"


using namespace fnifi;
using namespace fnifi::expression;

Expression::Expression() {}

void Expression::build(const std::string& expr, const std::string& storingPath)
{
    if (_stored.is_open()) {
        _stored.close();
    }
    const auto filename = storingPath + GetHash(expr) + ".fnifi";
    if (std::filesystem::exists(filename)) {
        _stored.open(filename, std::ios::in | std::ios::out |
                     std::ios::binary | std::ios::ate);
        _maxId = static_cast<fileId_t>(static_cast<size_t>(_stored.tellg())
                                       / sizeof(expr_t));
    } else {
        _stored = std::fstream(filename, std::ios::in | std::ios::out |
                               std::ios::binary | std::ios::trunc);
        _maxId = 0;
    }
    _storingPath = storingPath;
    _vars.clear();
    _handler =
        [&](const std::string& name) -> expr_t& {
            Variable* var;
            const auto pos = name.find('.');
            if (pos == std::string::npos) {
                /* this is a listed variable */
                const auto type = Variable::GetType(name);
                var = &Variable::Build(type, name, _storingPath.c_str());
            } else {
                /* this is an unkown variable */
                const auto type = Variable::GetType(name.substr(pos));
                var = &Variable::Build(type, name.substr(pos + 1),
                                       _storingPath.c_str());
            }
            _vars.push_back({var, 0});
            return _vars.back().ref;
        };
    _sxeval.build(expr, _handler);
}

expr_t Expression::run(const file::File* file, bool noCache) {
    if (!_stored.is_open()) {
        WLOG("Expression " << this
             << " has been called before being built. Aborting the call")
        return 0;
    }

    const auto id = file->getId();
    const auto pos = id * sizeof(expr_t);

    if (id <= _maxId) {
        if (!noCache) {
            /* the value may be saved */
            expr_t res;
            _stored.seekg(std::streamoff(pos));
            Deserialize(_stored, res);

            if (res != EMPTY_EXPR_T) {
                /* the value was saved */
                return res;
            }
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

void Expression::remove(fileId_t id) {
    if (!_stored.is_open()) {
        WLOG("Expression " << this
             << " has been called before being built. Aborting the call")
        return;
    }

    if (id <= _maxId) {
        _stored.seekp(id * sizeof(expr_t));
        Serialize(_stored, EMPTY_EXPR_T);
    }
}

expr_t Expression::getValue(const file::File* file) {
    /* fill the variables */
    for (auto& var : _vars) {
        var.ref = var.var->get(file);
    }

    /* run sxeval */
    return _sxeval.execute();
}

std::string Expression::GetHash(const std::string& s) {
    auto res = s;
    for (char c : "/\\:*?\"<>|") {
        std::replace(res.begin(), res.end(), c, '_');
    }
    return res;
}
