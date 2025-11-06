#include <fnifi/FNIFI.hpp>
#include <fnifi/Collection.hpp>
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
    std::vector<fnifi::Collection> colls;
    for (int i = 2; i < argc; ++i) {
        colls.push_back(fnifi::Collection(&conn, argv[i], argv[1]));
    }

    /* File indexing */
    fnifi::FNIFI fi(conns, colls, argv[1]);

    return 0;
}
