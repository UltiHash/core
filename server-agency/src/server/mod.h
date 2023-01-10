#ifndef SERVER_AGENCY_SERVER_MOD_H
#define SERVER_AGENCY_SERVER_MOD_H

#include <config/options.h>
#include <metrics/mod.h>
#include <cluster/mod.h>

#include <memory>


namespace uh::an::server
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const config::options& options,
        an::cluster::mod& cluster,
        an::metrics::mod& metrics);

    ~mod();

    void start();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
