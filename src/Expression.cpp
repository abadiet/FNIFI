#include "fnifi/expression/Expression.hpp"

#define EXPRESSIONS_DIRNAME "expressions"
#define RESULTS_FILENAME "results.fnifi"


using namespace fnifi;
using namespace fnifi::expression;

Expression::Expression(const std::filesystem::path& storingPath,
                       const std::vector<file::Collection*>& colls)
: _storingPath(storingPath)
{
    for (const auto& coll : colls) {
        /* create the folders if needed */
        const auto name = coll->getName();
        const auto dir = _storingPath / Hash(name);
        std::filesystem::create_directories(dir);

        /* fill _stored */
        _storedColls.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(std::fstream(), 0, dir)
        );
    }
}

Expression::~Expression() {
    for (auto& stored : _storedColls) {
        if (stored.second.file.is_open()) {
            stored.second.file.close();
        }
    }
}

void Expression::build(const std::string& expr) {
    /* update _stored */
    for (auto& s : _storedColls) {
        /* close previous results.fnifi file if needed */
        if (s.second.file.is_open()) {
            s.second.file.close();
            s.second.maxId = 0;
        }

        /* create directory if needed */
        const auto dir = s.second.path / EXPRESSIONS_DIRNAME / Hash(expr);
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

    /* build sxeval */
    _vars.clear();
    _handler =
        [&](const std::string& name) -> expr_t& {
            _vars.push_back({&Variable::Build(name), 0});
            return _vars.back().ref;
        };
    _sxeval.build(expr, _handler);
}

expr_t Expression::run(const file::File* file, bool noCache) {
    /* dirty way to check if Expression::build has been called */
    if (_storedColls.size() == 0 ||
        !_storedColls.begin()->second.file.is_open())
    {
        WLOG("Expression " << this
             << " has been called before being built. Aborting the call")
        return 0;
    }

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

    const auto res = getValue(file, noCache);
    stored->second.file.seekp(std::streamoff(pos));
    Serialize(stored->second.file, res);

    return res;
}

void Expression::remove(fileId_t id, const std::string& collectionName) {
    if (_storedColls.size()) {
        WLOG("Expression " << this
             << " has been called before being built. Aborting the call")
        return;
    }

    /* get the associated stored file */
    const auto stored = _storedColls.find(collectionName);
    if (stored == _storedColls.end()) {
        ELOG("Expression " << this << " has been called on a file that belongs"
             " to an unknown Collection (" << collectionName
             << ") Aborting the call.")
        return;
    }

    if (id <= stored->second.maxId) {
        stored->second.file.seekp(id * sizeof(expr_t));
        Serialize(stored->second.file, EMPTY_EXPR_T);
    }
}

expr_t Expression::getValue(const file::File* file, bool noCache) {
    /* fill the variables */
    for (auto& var : _vars) {
        var.ref = var.var->get(file, noCache);
    }

    /* run sxeval */
    return _sxeval.execute();
}
