#ifndef SERVER_AGENCY_SERVER_MOD_H
#define SERVER_AGENCY_SERVER_MOD_H

#include <net/plain_server.h>

#include <metrics/mod.h>
#include <cluster/mod.h>
#include <persistence/mod.h>

#include <memory>


namespace uh::an::server
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const net::server_config& config,
        an::cluster::mod& cluster,
        an::metrics::mod& metrics);

    ~mod();

    void start();
    void stop();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
