#include "mod.h"

#include <config.hpp>

#include <server/protocol_factory.h>
#include <protocol/client_factory.h>
#include <net/server.h>
#include <net/tls_server.h>
#include <net/plain_socket.h>


namespace uh::an::server
{

namespace
{

// ---------------------------------------------------------------------

uh::protocol::client_factory_config make_cf_config()
{
    std::stringstream s;

    s << PROJECT_NAME << " " << PROJECT_VERSION;

    return uh::protocol::client_factory_config{ .client_version = s.str() };
}

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
         an::metrics::metrics& metrics);

    boost::asio::io_context io;
    net::plain_socket_factory socket_factory;
    protocol::client_factory_config cf_config;
    protocol::client_factory client_factory;
    protocol::client_pool clients;
    protocol_factory pf;
    std::unique_ptr<net::server> server;
};

// ---------------------------------------------------------------------

mod::impl::impl(const config::options& options,
                an::metrics::metrics& metrics)
    : io(),
      socket_factory(io, "localhost", 12345),
      cf_config(make_cf_config()),
      client_factory(socket_factory, cf_config),
      clients(client_factory, 10),
      pf(clients, metrics),
      server(make_server(options.server().config(), pf))
{
}

// ---------------------------------------------------------------------

mod::mod(const config::options& options,
         an::metrics::metrics& metrics)
    : m_impl(std::make_unique<mod::impl>(options, metrics))
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
