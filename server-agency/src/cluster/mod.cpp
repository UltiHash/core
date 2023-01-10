#include "mod.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <protocol/client_factory.h>
#include <net/plain_socket.h>
#include <util/exception.h>


using namespace boost::asio;
using namespace uh::protocol;
using namespace uh::net;

namespace uh::an::cluster
{

namespace
{

// ---------------------------------------------------------------------

client_factory_config make_cf_config()
{
    std::stringstream s;

    s << PROJECT_NAME << " " << PROJECT_VERSION;

    return client_factory_config{ .client_version = s.str() };
}

// ---------------------------------------------------------------------

node_ref make_node_ref(const config::node_config& nc)
{
    std::stringstream s;
    s << nc.hostname << ":" << nc.port;
    return s.str();
}

// ---------------------------------------------------------------------

std::unique_ptr<util::factory<net::socket>> make_socket_factory(
    io_context& io,
    const config::node_config& nc)
{
    switch (nc.method)
    {
        case config::connection_method::plain:
            return std::make_unique<plain_socket_factory>(io, nc.hostname, nc.port);
        case config::connection_method::tls:
            THROW(util::exception, "TLS not implemented yet");  // TODO
    }

    THROW(util::exception, "unknown connection method");
}

// ---------------------------------------------------------------------

std::unique_ptr<client_pool> make_client_pool(
    io_context& io,
    const config::node_config& nc,
    client_factory_config& cf)
{
    return std::make_unique<client_pool>(
        std::make_unique<client_factory>(make_socket_factory(io, nc), cf),
        nc.pool_size);
}

// ---------------------------------------------------------------------

std::map<node_ref, std::unique_ptr<client_pool>> make_nodes(
    io_context& io,
    const config& cfg)
{
    std::map<node_ref, std::unique_ptr<client_pool>> rv;

    auto cf_config = make_cf_config();

    for (const auto& nc : cfg.nodes)
    {
        rv[make_node_ref(nc)] = make_client_pool(io, nc, cf_config);
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const config& cfg);

    io_context io;
    std::map<node_ref, std::unique_ptr<client_pool>> nodes;
};

// ---------------------------------------------------------------------

mod::impl::impl(const config& cfg)
    : nodes(make_nodes(io, cfg))
{
}

// ---------------------------------------------------------------------

mod::mod(const config& cfg)
    : m_impl(std::make_unique<impl>(cfg))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting cluster module";
}

// ---------------------------------------------------------------------

client_pool& mod::node(const node_ref& ref)
{
    auto node = m_impl->nodes.find(ref);
    if (node == m_impl->nodes.end())
    {
        THROW(util::exception, std::string("node not found: ") + ref);
    }

    return *node->second;
}

// ---------------------------------------------------------------------

std::list< std::pair<node_ref, std::size_t> > mod::bc_free_space()
{
    std::list< std::pair<node_ref, std::size_t> > rv;

    for (const auto& node : m_impl->nodes)
    {
        try
        {
            rv.push_back(
                std::make_pair(node.first, node.second->get()->free_space())
            );
        }
        catch (const std::exception& e)
        {
            WARNING << "could not retrieve free space for `" << node.first
                << "`: " << e.what();
        }
    }

    return rv;
}

// ---------------------------------------------------------------------

blob mod::bc_read_block(const blob& hash)
{
    for (const auto& node : m_impl->nodes)
    {
        try
        {
            return node.second->get()->read_block(hash);
        }
        catch (const std::exception& e)
        {
            INFO << "hash not known to " << node.first << ": " << e.what();
        }
    }

    THROW(util::exception, "hash could not be found in cluster");
}

// ---------------------------------------------------------------------

} // namespace uh::an::cluster
