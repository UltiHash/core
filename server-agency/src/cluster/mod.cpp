#include "mod.h"
#include "sample_hash_routing.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <protocol/client_factory.h>
#include <net/plain_socket.h>
#include <util/exception.h>
#include <forward_list>


using namespace boost::asio;
using namespace uh::protocol;
using namespace uh::net;

namespace uh::an::cluster
{

    namespace
    {

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

        void stop(const std::string& reason);

        io_context io;
        std::unordered_map<node_ref, std::unique_ptr<client_pool>> nodes;
        std::unique_ptr <routing_interface> m_routing;

    };

// ---------------------------------------------------------------------

    mod::impl::impl(const config& cfg)
            : nodes(make_nodes(io, cfg)),
              m_routing(std::make_unique <sample_hash_routing> (nodes))
    {
    }

// ---------------------------------------------------------------------

    void mod::impl::stop(const std::string& reason)
    {
        for (const auto& pair : nodes)
        {
            pair.second->quit_all(reason);
        }
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

    void mod::stop()
    {
        INFO << "stopping cluster module";

        m_impl->stop("server agency stopped");
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

// ---------------------------------------------------------------------

    uh::protocol::write_chunks::response mod::write_chunks(const write_chunks::request &req) {
        std::map <client_pool *, chunks_meta_data> conn_blocks_map;

        size_t offset = 0;

        for (std::size_t i = 0; i < req.chunk_sizes.size(); ++i) {
            const auto chunk_size = req.chunk_sizes[i];
            std::span <const char> block {req.data.data() + offset, chunk_size};
            auto &client_connections = m_impl->m_routing->route_data(block);
            auto &chunks_md = conn_blocks_map [&client_connections];
            chunks_md.data.insert(conn_blocks_map [&client_connections].data.end(), block.data(), block.data() + block.size());
            chunks_md.chunk_sizes.emplace_back(chunk_size);
            chunks_md.chunk_indices.emplace_back(i);
            offset += chunk_size;
        }

        // TODO this should be done in different threads
        uh::protocol::write_chunks::response total_res;
        total_res.hashes.resize(req.chunk_sizes.size() * 64);
        total_res.effective_size.resize(req.chunk_sizes.size());
        for (auto &conn_blocks: conn_blocks_map) {
            auto res = conn_blocks.first->get()->write_chunks ({conn_blocks.second.chunk_sizes, conn_blocks.second.data});
            std::size_t dn_index = 0;
            std::size_t index = 0;
            for (const auto total_index: conn_blocks.second.chunk_indices) {
                total_res.effective_size[total_index] = res.effective_size[index];
                std::memcpy (total_res.hashes.data() + total_index * 64, res.hashes.data() + (index * 64), 64);
                ++index;
            }
        }
        return total_res;

    }

// ---------------------------------------------------------------------

    uh::protocol::read_chunks::response mod::read_chunks(const read_chunks::request &req) {

        std::map <client_pool *, std::vector <char>> conn_hashes_map;

        auto routed_hash_offsets = m_impl->m_routing->route_hashes(req.hashes);

        for (const auto &connection_hash_off_pair: routed_hash_offsets) {

            auto& conn_hashes = conn_hashes_map[connection_hash_off_pair.first];
            conn_hashes.reserve(connection_hash_off_pair.second.size() * 64);

            for (const auto req_hash_off: connection_hash_off_pair.second) {
                conn_hashes.insert(conn_hashes.end(), req.hashes.data() + req_hash_off, req.hashes.data() + req_hash_off + 64);
            }
        }

        std::forward_list <uh::protocol::read_chunks::response> responses;
        std::map <std::size_t, std::span <char>> hash_offset_data_map;

        // TODO this should be done in different threads
        for (auto &conn_hashes: conn_hashes_map) {

            const auto &conn_hash_offsets = routed_hash_offsets [conn_hashes.first];
            responses.emplace_front(conn_hashes.first->get()->read_chunks ({conn_hashes.second}));
            auto& resp = responses.front();
            auto& resp_data = std::get<0>(resp.data);

            size_t offset = 0;
            size_t chunk_size_id = 0;
            for (const auto hash_offset: conn_hash_offsets) {
                const auto chunk_size = resp.chunk_sizes[chunk_size_id ++];
                hash_offset_data_map[hash_offset] = std::span<char>{ resp_data.data() + offset, chunk_size };
                offset += chunk_size;
            }
        }

        uh::protocol::read_chunks::response total_resp;
        auto& data = std::get<0>(total_resp.data);
        for (const auto &hash_offset_data_pair: hash_offset_data_map) {
            data.insert(data.end(), hash_offset_data_pair.second.begin(), hash_offset_data_pair.second.end());
        }

        return total_resp;
    }

// ---------------------------------------------------------------------

    write_key_value::response mod::write_kv(const write_key_value::request &request) {
        // TODO this uses dummy routing

        auto &client_connections = m_impl->m_routing->route_data("dummy data");

        return client_connections.get()->write_kv (request);
    }

// ---------------------------------------------------------------------

    read_key_value::response mod::read_kv(const read_key_value::request &request) {
        auto &client_connections = m_impl->m_routing->route_data("dummy data");
        return client_connections.get()->read_kv (request);
    }

// ---------------------------------------------------------------------

} // namespace uh::an::cluster
