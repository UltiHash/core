#include "mod.h"

#include <config.hpp>

#include <server/protocol_factory.h>
#include <net/server.h>
#include <net/tls_server.h>


namespace uh::dbn::server
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
    impl(const net::server_config& config,
         dbn::storage::mod& storage,
         dbn::metrics::mod& metrics);

    boost::asio::io_context io;
    std::unique_ptr<net::server> server;
    net::server_info serv_info;
    protocol_factory pf;

};

// ---------------------------------------------------------------------

mod::impl::impl(const net::server_config& config,
                dbn::storage::mod& storage,
                dbn::metrics::mod& metrics)
    : io(),
      server(make_server(config, pf)),
      serv_info (*server),
      pf(storage, metrics.protocol(), serv_info)
{
}

// ---------------------------------------------------------------------

mod::mod(const net::server_config& config,
         dbn::storage::mod& storage,
         dbn::metrics::mod& metrics)
    : m_impl(std::make_unique<mod::impl>(config, storage, metrics))
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

} // namespace uh::dbn::server
