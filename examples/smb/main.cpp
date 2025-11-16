#include <fnifi/FNIFI.hpp>
#include <fnifi/file/Collection.hpp>
#include <fnifi/file/File.hpp>
#include <fnifi/connection/Local.hpp>
#include <fnifi/connection/SMB.hpp>
#include <fnifi/connection/Relative.hpp>
#include <iostream>


int main(int argc, char** argv) {
    if (argc != 8) {
        std::cout << "Usage: " << argv[0]
            << " <tmpPath> <SMBServer> <SMBShare> <SMBUsername> <SMBPassword> "
            << "<SMBStoringPath> <SMBIndexingPath>";
        return 1;
    }

    /* Collections */
    fnifi::connection::Local localConn;
    fnifi::connection::SMB smbConn(argv[2], argv[3], "", argv[4], argv[5]
                                   );
    fnifi::connection::Relative storing(&smbConn, argv[6]);
    fnifi::connection::Relative indexing(&smbConn, argv[7]);
    localConn.connect();
    smbConn.connect();
    storing.connect();
    indexing.connect();
    fnifi::file::Collection coll(&indexing, &storing, argv[1]);
    std::vector<fnifi::file::Collection*> colls = {&coll};

    /* File indexing */
    fnifi::FNIFI fi(colls, &localConn, argv[1]);

    /* Defragment to optimize disk usage */
    fi.defragment();

    /* Loop over the files */
    for (const auto& file : fi) {
        std::cout << file->getPath() << ": ";
        file->getMetadata(std::cout, fnifi::file::MetadataType::EXIF,
                          "Exif.Image.Model");
        std::cout << std::endl;
    }

    /* Cleaning */
    localConn.disconnect();
    smbConn.disconnect();
    storing.disconnect();
    indexing.disconnect();

    return 0;
}
