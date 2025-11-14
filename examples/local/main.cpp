#include <fnifi/FNIFI.hpp>
#include <fnifi/file/Collection.hpp>
#include <fnifi/file/File.hpp>
#include <fnifi/connection/Local.hpp>
#include <iostream>


int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <storingPath> <indexingPath0> "
            "... <indexingPathN>";
        return 1;
    }

    /* Connections */
    fnifi::connection::Local conn;
    std::vector<fnifi::connection::IConnection*> conns({&conn});

    /* Collections */
    std::vector<fnifi::file::Collection*> colls;
    for (int i = 2; i < argc; ++i) {
        colls.push_back(new fnifi::file::Collection(&conn, argv[i], argv[1]));
    }

    /* File indexing */
    fnifi::FNIFI fi(conns, colls, &conn, argv[1]);

    /* Loop over the files */
    for (const auto& file : fi) {
        std::cout << file->getPath() << ": ";
        file->getMetadata(std::cout, fnifi::file::MetadataType::EXIF,
                          "Exif.Image.Model");
        std::cout << std::endl;
    }

    /* Cleaning */
    for (auto& coll : colls) {
        delete coll;
    }

    return 0;
}
