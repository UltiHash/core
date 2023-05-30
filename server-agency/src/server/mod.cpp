#include "mod.h"

#include <config.hpp>

#include <server/protocol_factory.h>
#include <net/server.h>
#include <net/tls_server.h>
#include <logging/logging_boost.h>
#include <future>


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
    impl(const net::server_config& config,
         an::cluster::mod& cluster,
         an::metrics::mod& metrics);

    boost::asio::io_context io;

    struct server_wrapper
    {
        explicit server_wrapper(std::unique_ptr<net::server>&& server);
        void stop();

        std::unique_ptr<net::server> server;
        std::future<void> server_future;
    };

    server_wrapper server_wrapper;
    net::server_info serv_info;
    protocol_factory pf;

};

// ---------------------------------------------------------------------

mod::impl::server_wrapper::server_wrapper(std::unique_ptr<net::server>&& serv) : server(std::move(serv))
{
}

// ---------------------------------------------------------------------

void mod::impl::server_wrapper::stop()
{
    server->stop();
    server_future.get();
}

// ---------------------------------------------------------------------

mod::impl::impl(const net::server_config& config,
                an::cluster::mod& cluster,
                an::metrics::mod& metrics)
    : io(),
      server_wrapper(make_server(config, pf)),
      serv_info (*server_wrapper.server),
      pf(cluster, metrics.client(), metrics.protocol(), serv_info)
{
}

// ---------------------------------------------------------------------

mod::mod(const net::server_config& config,
         an::cluster::mod& cluster,
         an::metrics::mod& metrics)
    : m_impl(std::make_unique<mod::impl>(config, cluster, metrics))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "            starting server";
    m_impl->server_wrapper.server_future = std::async(std::launch::async,
                                                      [&]() { m_impl->server_wrapper.server->run(); });
}

// ---------------------------------------------------------------------

void mod::stop()
{
    INFO << "             stopping server";
    m_impl->server_wrapper.stop();
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
