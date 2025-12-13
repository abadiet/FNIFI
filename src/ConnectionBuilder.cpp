#include "fnifi/connection/ConnectionBuilder.hpp"

#include "fnifi/connection/Local.hpp"
#include "fnifi/connection/SMB.hpp"
#include "fnifi/connection/Relative.hpp"

#define SEP std::string("\xFF")


using namespace fnifi;
using namespace fnifi::connection;

std::unordered_map<std::string, IConnection*> ConnectionBuilder::_built;

IConnection* ConnectionBuilder::GetLocal(const ConnectionBuilder::Options& opt)
{
    IConnection* conn;
    const auto hsh = "Local" + SEP;
    const auto pos = _built.find(hsh);
    if (pos == _built.end()) {
        const auto res = _built.insert({hsh, new Local()});
        conn = res.first->second;
        conn->connect(opt.maxConnectionTry);
    } else {
        conn = pos->second;
    }

    return SetupOptions(conn, hsh, opt);
}

IConnection* ConnectionBuilder::GetSMB(const std::string& server,
                                       const std::string& share,
                                       const std::string& username,
                                       const std::string& password,
                                       const ConnectionBuilder::Options& opt)
{
    IConnection* conn;
    const auto hsh = "SMB" + SEP + server + SEP + share + SEP + username + SEP
        + password + SEP;
    const auto pos = _built.find(hsh);
    if (pos == _built.end()) {
        const auto res = _built.insert({hsh,
            new SMB(server, share, username, password)});
        conn = res.first->second;
        conn->connect(opt.maxConnectionTry);
    } else {
        conn = pos->second;
    }

    return SetupOptions(conn, hsh, opt);
}

void ConnectionBuilder::Free() {
    for (auto& conn : _built) {
        conn.second->disconnect();
         delete conn.second;
    }
}

IConnection* ConnectionBuilder::SetupOptions(IConnection* conn,
    const std::string& hsh, const ConnectionBuilder::Options& opt)
{
    const auto opthsh = opt.relativePath + SEP;
    const auto fullhsh = hsh + opthsh;

    const auto pos = _built.find(fullhsh);
    if (pos == _built.end()) {
        const auto res = _built.insert({fullhsh,
            new Relative(conn, opt.relativePath)});
        conn = res.first->second;
        conn->connect(opt.maxConnectionTry);
    } else {
        conn = pos->second;
    }

    return conn;
}
