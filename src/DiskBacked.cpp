#include "fnifi/expression/DiskBacked.hpp"
#include <csignal>


using namespace fnifi;
using namespace fnifi::expression;

void DiskBacked::Uncache(const utils::SyncDirectory& storing,
                         const std::filesystem::path& path, fileId_t id)
{
    DLOG("DiskBacked", "(static)", "Uncaching file id " << id
         << " for directory " << path)

    if (std::filesystem::exists(path)) {
        for (const auto& dir : std::filesystem::directory_iterator(path)) {
            if (dir.is_directory()) {
                /* write an empty results on the id position */
                auto file = storing.open(path / dir.path().filename());
                file.seekp(id * sizeof(expr_t));
                utils::Serialize(file, EMPTY_EXPR_T);
                file.push();
                file.close();
            }
        }
    }
}

DiskBacked::DiskBacked(const std::string& key,
           const utils::SyncDirectory& storing,
           const std::vector<file::Collection*>& colls,
           const std::filesystem::path& parentDirName)
: _keyHash(utils::Hash(key)), _storing(storing), _parentDirName(parentDirName)
{
    DLOG("DiskBacked", this, "Instanciation for key \"" << key << "\"")

    for (const auto& coll : colls) {
        addCollection(*coll);
    }
}

DiskBacked::~DiskBacked() {
    for (auto& stored : _storedColls) {
        if (stored.second.file->is_open()) {
            stored.second.file->close();
        }
    }
}

void DiskBacked::addCollection(const file::Collection& coll) {
    /* create or open the results.fnifi file */
    const auto name = coll.getName();
    const auto filename = utils::Hash(name) / _parentDirName / _keyHash;
    bool ate = false;
    if (_storing.exists(filename)) {
        ate = true;
    }

    /* fill _stored */
    const auto& storedColl = _storedColls.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(name),
        std::forward_as_tuple(
            std::make_unique<utils::SyncDirectory::FileStream>(
                _storing, filename, ate),
            0
        )
    );

    if (ate) {
        /* the file already existed */
        storedColl.first->second.maxId = static_cast<fileId_t>(
            static_cast<size_t>(storedColl.first->second.file->tellg())
            / sizeof(expr_t));
    }
}

expr_t DiskBacked::get(const file::File* file) {
    DLOG("DiskBacked", this, "Retrieving result for File " << file)

    /* get the associated stored file */
    const auto stored = _storedColls.find(file->getCollectionName());
    if (stored == _storedColls.end()) {
        ELOG("DiskBacked ", this, "Called on a file that belongs to an unknown"
             " Collection (" << file->getCollectionName() << ") Aborting the "
             "call.")
        return 0;
    }

    const auto id = file->getId();
    const auto pos = id * sizeof(expr_t);

    if (stored->second.file->pull()) {
        /* update maxId */
        stored->second.file->seekg(0, std::ios::end);
        stored->second.maxId = static_cast<fileId_t>(static_cast<size_t>(
            stored->second.file->tellg()) / sizeof(expr_t));
    }

    if (id <= stored->second.maxId) {
        /* the value may be saved */
        expr_t res;
        stored->second.file->seekg(std::streamoff(pos));
        utils::Deserialize(*stored->second.file, res);

        if (res != EMPTY_EXPR_T) {
            /* the value was saved */
            return res;
        }

        stored->second.file->seekg(std::streamoff(pos));
    } else {
        /* filling the file up to the position of the value */
        stored->second.file->seekp(0, std::ios::end);
        for (auto i = stored->second.maxId; i < id; ++i) {
            /* TODO avoid multiples std::ofstream::write calls */
            utils::Serialize(*stored->second.file, EMPTY_EXPR_T);
        }
        stored->second.maxId = id;
    }

    DLOG("DiskBacked", this, "Results for File " << file << " was not cached")

    const auto res = getValue(file);
    stored->second.file->seekp(std::streamoff(pos));
    utils::Serialize(*stored->second.file, res);

    stored->second.file->push();

    return res;
}

void DiskBacked::disableSync(const std::string& collName, bool pull) {
    const auto stored = _storedColls.find(collName);
    if (stored == _storedColls.end()) {
        ELOG("DiskBacked", this, "Lock called on a file that belongs to an "
             "unknown Collection (" << collName << ") Aborting the call.")
        return;
    }

    stored->second.file->disableSync(pull);
}

void DiskBacked::enableSync(const std::string& collName, bool push) {
    const auto stored = _storedColls.find(collName);
    if (stored == _storedColls.end()) {
        ELOG("DiskBacked", this, "Unlock called on a file that belongs to an "
            "unknown Collection (" << collName << ") Aborting the call.")
        return;
    }

    stored->second.file->enableSync(push);
}
