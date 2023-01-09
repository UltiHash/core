#include "mod.h"

#include <config.hpp>

#include <server/protocol_factory.h>
#include <net/server.h>
#include <net/tls_server.h>


namespace uh::an::server
{

namespace
{

// ---------------------------------------------------------------------

std::unique_ptr<net::server> make_server(
    const net::server_config& config,
    protocol_factory& pf)
{
    if (!config.tls_chain.empty())
    {
        return std::make_unique<net::tls_server>(config, pf);
    }
    else
    {
        return std::make_unique<net::plain_server>(config, pf);
    }
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const config::options& options,
         an::cluster::mod& cluster,
         an::metrics::metrics& metrics);

    boost::asio::io_context io;
    protocol_factory pf;
    std::unique_ptr<net::server> server;
};

// ---------------------------------------------------------------------

mod::impl::impl(const config::options& options,
                an::cluster::mod& cluster,
                an::metrics::metrics& metrics)
    : io(),
      pf(cluster, metrics),
      server(make_server(options.server().config(), pf))
{
}

// ---------------------------------------------------------------------

mod::mod(const config::options& options,
         an::cluster::mod& cluster,
         an::metrics::metrics& metrics)
    : m_impl(std::make_unique<mod::impl>(options, cluster, metrics))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting server";
    m_impl->server->run();
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
