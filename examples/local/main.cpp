#include <fnifi/FNIFI.hpp>
#include <fnifi/file/Collection.hpp>
#include <fnifi/file/File.hpp>
#include <fnifi/connection/Local.hpp>
#include <fnifi/connection/Relative.hpp>
#include <iostream>


int main(int argc, char** argv) {
    if (argc < 3 || argc % 2 != 0) {
        std::cout << "Usage: " << argv[0]
            << " <tmpPath> <storingPath0> <indexingPath0> ... <storingPathN>"
            " <indexingPathN>";
        return 1;
    }

    /* Collections */
    fnifi::connection::Local localConn;
    localConn.connect();
    std::vector<fnifi::connection::IConnection*> conns;
    std::vector<fnifi::file::Collection*> colls;
    for (int i = 2; i < argc; i += 2) {
        const auto storing = new fnifi::connection::Relative(&localConn,
                                                             argv[i]);
        const auto indexing = new fnifi::connection::Relative(&localConn,
                                                              argv[i + 1]);
        storing->connect();
        indexing->connect();
        conns.push_back(storing);
        conns.push_back(indexing);
        colls.push_back(new fnifi::file::Collection(indexing, storing, argv[1])
                        );
    }

    /* File indexing */
    fnifi::FNIFI fi(colls, argv[1]);

    /* Defragment to optimize disk usage */
    fi.defragment();

    /* Loop over the files */
    std::cout << "Randomly loop over all the files:" << std::endl;
    for (const auto file : fi) {
        const auto ctime = file->getStats().st_ctimespec;
        std::cout << file->getPath() << ": ctime="
            << ctime.tv_sec << "s and " << ctime.tv_nsec
            << "ns, Exif.Image.Model=";
        file->getMetadata(std::cout, fnifi::expression::Type::EXIF,
                          "Exif.Image.Model");
        std::cout << std::endl;
    }

    /* sort the files by ctime */
    fi.sort("(+ ctime 0)");
    std::cout << std::endl << "Sorting by ctime:" << std::endl;
    for (const auto file : fi) {
        const auto ctime = file->getStats().st_ctimespec;
        std::cout << file->getPath() << ": ctime="
            << S_TO_NS(ctime.tv_sec) + ctime.tv_nsec << "ns" << std::endl;
    }

    /* filter the files to keep ones with a odd ctime */
    fi.filter("(% ctime 2)");
    std::cout << std::endl << "Filtering to keep odd ctimes:" << std::endl;
    for (const auto file : fi) {
        const auto ctime = file->getStats().st_ctimespec;
        std::cout << file->getPath() << ": ctime="
            << S_TO_NS(ctime.tv_sec) + ctime.tv_nsec << "ns" << std::endl;
    }

    /* Cleaning */
    for (auto& coll : colls) {
        delete coll;
    }
    for (auto& conn : conns) {
        conn->disconnect();
    }
    localConn.disconnect();
    for (auto& conn : conns) {
        delete conn;
    }
    return 0;
}
