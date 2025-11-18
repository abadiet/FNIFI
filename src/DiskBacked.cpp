#include "fnifi/expression/DiskBacked.hpp"
#include <csignal>

#define RESULTS_FILENAME "results.fnifi"


using namespace fnifi;
using namespace fnifi::expression;

void DiskBacked::Uncache(const std::filesystem::path& path, fileId_t id) {
    DLOG("DiskBacked is uncaching file's id " << id << " for directory "
         << path)
    if (std::filesystem::exists(path)) {
        for (const auto& dir : std::filesystem::directory_iterator(path)) {
            if (dir.is_directory()) {
                const auto filename = dir.path() / RESULTS_FILENAME;
                if (std::filesystem::exists(filename)) {
                    /* write an empty results on the id position */
                    std::fstream file(filename, std::ios::in | std::ios::out |
                                       std::ios::binary);
                    file.seekp(id * sizeof(expr_t));
                    Serialize(file, EMPTY_EXPR_T);
                    file.close();
                } else {
                    WLOG("DiskBacked did not find the " << RESULTS_FILENAME
                         << " for directory " << dir)
                }
            }
        }
    }
}

DiskBacked::DiskBacked(const std::string& key,
           const std::filesystem::path& storingPath,
           const std::vector<file::Collection*>& colls,
           const std::filesystem::path& parentDirName)
{
    for (const auto& coll : colls) {
        /* create the folders if needed */
        const auto name = coll->getName();
        const auto dir = storingPath / Hash(name) / parentDirName / Hash(key);
        std::filesystem::create_directories(dir);

        /* create or open the results.fnifi file */
        const auto filename = dir / RESULTS_FILENAME;
        std::ios_base::openmode flag;
        if (std::filesystem::exists(filename)) {
            flag = std::ios::ate;
        } else {
            flag = std::ios::trunc;
        }

        /* fill _stored */
        const auto& storedColl = _storedColls.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(
                std::make_unique<std::fstream>(filename, std::ios::in |
                                               std::ios::out |
                                               std::ios::binary | flag),
                0,
                dir
            )
        );

        if (flag == std::ios::ate) {
            /* the file already existed */
            storedColl.first->second.maxId = static_cast<fileId_t>(
                static_cast<size_t>(storedColl.first->second.file->tellg())
                / sizeof(expr_t));
        }
    }
}

DiskBacked::~DiskBacked() {
    for (auto& stored : _storedColls) {
        if (stored.second.file->is_open()) {
            stored.second.file->close();
        }
    }
}

expr_t DiskBacked::get(const file::File* file, bool noCache) {
    /* get the associated stored file */
    const auto stored = _storedColls.find(file->getCollectionName());
    if (stored == _storedColls.end()) {
        ELOG("DiskBacked " << this << " has been called on a file that belongs"
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
            stored->second.file->seekg(std::streamoff(pos));
            Deserialize(*stored->second.file, res);

            if (res != EMPTY_EXPR_T) {
                /* the value was saved */
                return res;
            }
        }
    } else {
        /* filling the file up to the position of the value */
        stored->second.file->seekp(0, std::ios::end);
        for (auto i = stored->second.maxId; i < id; ++i) {
            /* TODO avoid multiples std::ofstream::write calls */
            Serialize(*stored->second.file, EMPTY_EXPR_T);
        }
        stored->second.maxId = id;
    }

    const auto res = getValue(file, noCache);
    stored->second.file->seekp(std::streamoff(pos));
    Serialize(*stored->second.file, res);

    return res;
}

void update();
