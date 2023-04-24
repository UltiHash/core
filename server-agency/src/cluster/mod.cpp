#include "mod.h"
#include "sample_hash_routing.h"

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

class connection_device : public io::device
{
public:
    connection_device(protocol::client_pool::handle&& h,
                      std::unique_ptr<io::device>&& dev)
        : m_dev(std::move(dev)),
          m_handle(std::move(h))
    {
    }

    std::streamsize write(std::span<const char> buffer) override
    {
        return m_dev->write(buffer);
    }

    std::streamsize read(std::span<char> buffer) override
    {
        return m_dev->read(buffer);
    }

    bool valid() const override
    {
        return m_dev->valid();
    }

private:
    protocol::client_pool::handle m_handle;
    std::unique_ptr<io::device> m_dev;
};

// ---------------------------------------------------------------------

class alloc : public uh::protocol::allocation
{
public:
    alloc(uh::protocol::client_pool::handle&& handle, std::size_t size)
        : m_handle(std::move(handle)),
          m_alloc(m_handle->allocate(size))
    {
    }

    virtual io::device& device() override
    {
        return m_alloc->device();
    }

    virtual block_meta_data persist() override
    {
        return m_alloc->persist();
    }

private:
    uh::protocol::client_pool::handle m_handle;
    std::unique_ptr<uh::protocol::allocation> m_alloc;
};

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

std::unordered_map<node_ref, std::unique_ptr<client_pool>> make_nodes(
    io_context& io,
    const config& cfg)
{
    std::unordered_map<node_ref, std::unique_ptr<client_pool>> rv;

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
    explicit impl(const config& cfg);

    io_context io;
    std::unordered_map<node_ref, std::unique_ptr<client_pool>> nodes;
    std::unique_ptr <routing_interface> m_routing;

};

// ---------------------------------------------------------------------

mod::impl::impl(const config& cfg)
    : nodes(make_nodes(io, cfg)),
    m_routing(std::make_unique <sample_hash_routing> (sample_hash_routing (nodes)))
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
            rv.emplace_back(node.first, node.second->get()->free_space());
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

std::unique_ptr<io::device> mod::bc_read_block(const blob& hash)
{
    for (const auto& node : m_impl->nodes)
    {
        try
        {
            auto conn = node.second->get();
            auto dev = conn->read_block(hash);
            return std::make_unique<connection_device>(std::move(conn), std::move(dev));
        }
        catch (const std::exception& e)
        {
            INFO << "hash not known to " << node.first << ": " << e.what();
        }
    }

    THROW(util::exception, "hash could not be found in cluster");
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> mod::allocate(std::size_t size)
{
    auto free_space = bc_free_space();
    if (free_space.empty())
    {
        THROW(util::exception, "no storage back-end configured");
    }

    free_space.sort([](auto& l, auto& r) { return l.second > r.second; });

    for (const auto& fs : free_space)
    {
        try
        {
            return std::make_unique<alloc>(node(fs.first).get(), size);
        }
        catch (const std::exception& e)
        {
            INFO << "allocation failed for `" << fs.first << "': " << e.what();
        }
    }

    THROW(util::exception, "insufficient space in back-end");
}

protocol::block_meta_data mod::write_small_block (std::span <char> buffer) {

    const auto &client_connections = m_impl->m_routing->route_data(buffer);
    return client_connections->get()->write_small_block(buffer);

}

// ---------------------------------------------------------------------

} // namespace uh::an::cluster
