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

    /* Indexation */
    fi.index();

    /* Defragment to optimize disk usage */
    fi.defragment();

    /* Loop over the files */
    {
        using T = unsigned long int;
        const auto ctime = fnifi::file::Info<T>::Build(
            &coll, fnifi::expression::Kind::CTIME, "");
        const auto lat = fnifi::file::Info<T>::Build(
            &coll, fnifi::expression::Kind::LATITUDE, "");
        const auto lon = fnifi::file::Info<T>::Build(
            &coll, fnifi::expression::Kind::LONGITUDE, "");
        ctime->disableSync();
        lat->disableSync();
        lon->disableSync();
        std::cout << "Randomly loop over all the files:" << std::endl;
        for (const auto file : fi) {
            T ct, la, lo;
            ctime->get(file, ct);
            lat->get(file, la);
            lon->get(file, lo);
            std::cout << file->getPath() << " " << ct << " " << " " << la << " " << lo << std::endl;
        }
        ctime->enableSync();
        lat->enableSync();
        lon->enableSync();
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
    fnifi::file::Info<int>::Free();  /* actually free every type */

    return 0;
}
