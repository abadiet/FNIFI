#include <fnifi/FNIFI.hpp>
#include <fnifi/file/Collection.hpp>
#include <fnifi/file/File.hpp>
#include <fnifi/connection/Local.hpp>
#include <fnifi/connection/Relative.hpp>
#include <iostream>


int main(int argc, char** argv) {
    if (argc != 4) {
        std::cout << "Usage: " << argv[0]
            << " <storingLocal> <storingServer> <indexingServer>";
        return 1;
    }

    /* Connections */
    fnifi::connection::Local localConn;
    localConn.connect();
    fnifi::connection::Relative storingServer(&localConn, argv[2]);
    storingServer.connect();
    fnifi::connection::Relative indexingServer(&localConn, argv[3]);
    indexingServer.connect();

    /* Synchronized directory (local processing and remote saving) */
    fnifi::utils::SyncDirectory storingLocal(&storingServer, argv[1]);

    /* File indexing */
    fnifi::FNIFI fi(storingLocal);

    /* Collections */
    fnifi::file::Collection coll(&indexingServer, storingLocal);
    fi.addCollection(coll);

    /* Defragment to optimize disk usage */
    fi.defragment();

    /* Loop over the files */
    std::cout << "Randomly loop over all the files:" << std::endl;
    for (const auto file : fi) {
        const auto ctime = file->getStats().st_ctimespec;
        std::cout << file->getPath() << ": ctime="
            << ctime.tv_sec << "s and " << ctime.tv_nsec
            << "ns, Exif.Image.Model=";
        file->getMetadata(std::cout, fnifi::expression::Kind::EXIF,
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
    localConn.disconnect();
    indexingServer.disconnect();
    storingServer.disconnect();

    return 0;
}
