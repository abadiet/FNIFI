#include <fnifi/FNIFI.hpp>
#include <fnifi/file/Collection.hpp>
#include <fnifi/file/File.hpp>
#include <fnifi/connection/ConnectionBuilder.hpp>
#include <iostream>
#include <ostream>


int main(int argc, char** argv) {
    if (argc != 9) {
        std::cout << "Usage: " << argv[0]
            << " <storingLocal> <storingServer> <indexingServer> <serverSMB> "
            "<shareSMB> <workgroupSMB> <usernameSMB> <passwordSMB>"
            << std::endl << "example: " << argv[0] << " ~/.fnifi .fnifi "
            "content 127.0.0.1 MyNAS WORKGROUP user0 mypassword" << std::endl
            << "will index smb://127.0.0.1/MyNAS/content, connected as user0 "
            "with password 'mypassword' and will save indexing in "
            "smb://127.0.0.1/MyNAS/.fnifi and locally in ~/.fnifi";
        return 1;
    }

    const std::string storingLocal(argv[1]);
    const std::string storingServer(argv[2]);
    const std::string indexingServer(argv[3]);
    const std::string serverSMB(argv[4]);
    const std::string shareSMB(argv[5]);
    const std::string workgroupSMB(argv[6]);
    const std::string usernameSMB(argv[7]);
    const std::string passwordSMB(argv[8]);

    /* Connections */
    auto indexingSer = fnifi::connection::ConnectionBuilder::GetSMB(
        serverSMB, shareSMB, workgroupSMB, usernameSMB, passwordSMB,
        { indexingServer });
    auto storingSer = fnifi::connection::ConnectionBuilder::GetSMB(
        serverSMB, shareSMB, workgroupSMB, usernameSMB, passwordSMB,
        { storingServer });

    /* Synchronized directory (local processing and remote saving) */
    fnifi::utils::SyncDirectory storingLoc(storingSer, storingLocal);

    /* File indexing */
    fnifi::FNIFI fi(storingLoc);

    /* Collection: the directory we want to index */
    fnifi::file::Collection coll(indexingSer, storingLoc);
    fi.addCollection(coll);

    /* Loop over the files */
    std::cout << "Randomly loop over all the files:" << std::endl;
    for (const auto file : fi) {
        std::cout << file->getPath() << std::endl;
    }

    fi.sort("(+ ctime 0)");

    fi.filter("(> ctime 2345678987)");

    /* Cleaning */
    fnifi::connection::ConnectionBuilder::Free();

    return 0;
}
