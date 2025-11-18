#include <fnifi/FNIFI.hpp>
#include <fnifi/file/Collection.hpp>
#include <fnifi/file/File.hpp>
#include <fnifi/connection/Local.hpp>
#include <fnifi/connection/SMB.hpp>
#include <fnifi/connection/Relative.hpp>
#include <iostream>


int main(int argc, char** argv) {
    if (argc != 10) {
        std::cout << "Usage: " << argv[0]
            << " <tmpPath> <SMBServer> <SMBShare> <SMBUsername> <SMBPassword> "
            << "<SMBStoringPath> <SMBIndexingPath> <LocalStoringPath> "
            << "<LocalIndexingPath>";
        return 1;
    }

    /* Collections */
    fnifi::connection::Local localConn;
    fnifi::connection::SMB smbConn(argv[2], argv[3], "", argv[4], argv[5]
                                   );
    fnifi::connection::Relative smbStoring(&smbConn, argv[6]);
    fnifi::connection::Relative smbIndexing(&smbConn, argv[7]);
    fnifi::connection::Relative localStoring(&localConn, argv[8]);
    fnifi::connection::Relative localIndexing(&localConn, argv[9]);
    localConn.connect();
    smbConn.connect();
    smbStoring.connect();
    smbIndexing.connect();
    localStoring.connect();
    localIndexing.connect();
    std::string smbTmpPath(argv[1]);
    smbTmpPath += "/smb";
    std::string localTmpPath(argv[1]);
    localTmpPath += "/local";
    fnifi::file::Collection smbColl(&smbIndexing, &smbStoring,
                                    smbTmpPath.c_str());
    fnifi::file::Collection localColl(&localIndexing, &localStoring,
                                      localTmpPath.c_str());
    std::vector<fnifi::file::Collection*> colls = {&smbColl, &localColl};

    /* File indexing */
    fnifi::FNIFI fi(colls, &localConn, argv[1]);

    /* Defragment to optimize disk usage */
    fi.defragment();

    /* Loop over the files */
    for (const auto file : fi) {
        std::cout << file->getPath() << ": ";
        file->getMetadata(std::cout, fnifi::expression::Type::EXIF,
                          "Exif.Image.Model");
        std::cout << std::endl;
    }

    /* Cleaning */
    localConn.disconnect();
    smbConn.disconnect();
    smbStoring.disconnect();
    smbIndexing.disconnect();
    localStoring.disconnect();
    localIndexing.disconnect();

    return 0;
}
