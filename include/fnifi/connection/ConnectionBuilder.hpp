#ifndef FNIFI_CONNECTION_CONNECTIONBUILDER_HPP
#define FNIFI_CONNECTION_CONNECTIONBUILDER_HPP

#include "fnifi/connection/IConnection.hpp"
#include <string>


namespace fnifi {
namespace connection {

class ConnectionBuilder {
public:
    struct Options {
        std::string relativePath;
    };

    static IConnection* GetLocal(const Options& opt);
    static IConnection* GetSMB(const std::string& server,
                               const std::string& share,
                               const std::string& username,
                               const std::string& password, const Options& opt
                               );
    static void Free();

private:
    static IConnection* SetupOptions(IConnection* conn, const std::string& hsh,
                                     const Options& opt);
    ConnectionBuilder() = delete;

    static std::unordered_map<std::string, IConnection*> _built;

};

}  /* namespace connection */
}  /* namespace fnifi */

#endif  /* FNIFI_CONNECTION_CONNECTIONBUILDER_HPP */
