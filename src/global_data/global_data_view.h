//
// Created by masi on 7/27/23.
//

#ifndef CORE_GLOBAL_DATA_VIEW_H
#define CORE_GLOBAL_DATA_VIEW_H

#include <map>
#include "data_node/data_node.h"
#include "network/client.h"
#include "ec.h"
#include "ec_factory.h"
#include "lru_cache.h"

namespace uh::cluster {

using namespace std::placeholders;

class global_data_view {


public:

    explicit global_data_view (const cluster_map& cmap):
                          m_cluster_map (cmap),
                          m_ec (ec_factory::make_ec (cmap.m_cluster_conf.global_data_view_conf.ec_algorithm)),
                          m_cache_l1 (cmap.m_cluster_conf.global_data_view_conf.read_cache_capacity_l1),
                          m_cache_l2 (cmap.m_cluster_conf.global_data_view_conf.read_cache_capacity_l2){
        sleep(2);
    }

    coro <address> write (const std::string_view& data) {
        auto index = m_data_node_index.load();
        auto new_val = (index + 1) % m_data_node_offsets.size();
        while (!m_data_node_index.compare_exchange_weak (index, new_val)) {
            index = m_data_node_index.load();
            new_val = (index + 1) % m_data_node_offsets.size();
        }

        auto m = m_data_node_offsets.at(index)->acquire_messenger();
        co_await m.get().send(WRITE_REQ, data);
        const auto message_header = co_await m.get().recv_header();
        auto resp = co_await m.get().recv_address(message_header);
        m.release();

        // l1 cache
        sspan <char> l1_buf (std::min (resp.first().size, m_cluster_map.m_cluster_conf.global_data_view_conf.l1_sample_size));
        std::memcpy (l1_buf.data(), data.data(), l1_buf.size());
        m_cache_l1.put (resp.first().pointer, std::move (l1_buf));
/*
        // l2 cache
        sspan <char> l2_buf (resp.first().size);
        std::memcpy (l2_buf.data(), data.data(), l2_buf.size());
        m_cache_l2.put (resp.first().pointer, std::move (l2_buf));
*/
        co_return std::move (resp);
    }

    sspan <char> read_l1_cache (const uint128_t pointer, const size_t size) {
        if (const auto c = m_cache_l1.get(pointer, nullptr); c.data() != nullptr) {
            if (c.size() >= size) [[likely]] {
                return c;
            }
        }
        return nullptr;
    }

    sspan <char> read_l2_cache (const uint128_t pointer, const size_t size) {
        if (const auto c = m_cache_l2.get(pointer, nullptr); c.data() != nullptr) {
            if (c.size() >= size) [[likely]] {
                return c;
            }
        }
        return nullptr;
    }

    coro <std::size_t> read (char* buffer, const uint128_t pointer, const size_t size) {
        const fragment frag {pointer, size};
        auto m = get_data_node (pointer)->acquire_messenger();
        co_await m.get().send_fragment(READ_REQ, frag);
        const auto h = co_await m.get().recv_header();
        m.get().register_read_buffer (buffer, h.size);
        co_await m.get().recv_buffers(h);
        m.release();

        // l1 cache
        sspan <char> l1_buf (std::min (h.size, m_cluster_map.m_cluster_conf.global_data_view_conf.l1_sample_size));
        std::memcpy (l1_buf.data(), buffer, l1_buf.size());
        m_cache_l2.put (pointer, std::move (l1_buf));

        // l2 cache
        sspan <char> l2_buf (h.size);
        std::memcpy (l2_buf.data(), buffer, h.size);
        m_cache_l2.put (pointer, std::move (l2_buf));

        co_return h.size;
    }

    /*
    coro <address> allocate (size_t total_size) {

        if (total_size % m_data_node_offsets.size() != 0) {
            throw std::length_error ("allocate size is not a multiple of the number of data nodes");
        }

        const size_t size = total_size / m_data_node_offsets.size();

        std::vector <std::shared_ptr <client>> nodes;
        nodes.reserve(m_data_node_offsets.size() + m_ec->get_acquired_ec_node_count());
        for (auto& node: m_data_node_offsets) {
            nodes.emplace_back(node.second);
        }
        nodes.insert(nodes.cend(), m_ec->get_ec_nodes().cbegin(), m_ec->get_ec_nodes().cend());
        address addr;
        auto bc = [&size] (auto m, auto id) -> coro <address> {
            m.get ().register_write_buffer (size);
            co_await m.get ().send_buffers (ALLOC_REQ);
            const auto h = co_await m.get ().recv_header ();
            auto resp = co_await m.get ().recv_address (h);
            co_return std::move (resp);
        };
        const auto resp = co_await broadcast_gather_custom <address> (*m_io_service, nodes, bc);
        for (const auto& r: resp) {
            addr.append_address(r.second);
        }

        co_return std::move (addr);
    }

    coro <void> cancel_allocation (const address& alloc) {
        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::vector <std::shared_ptr <client>> nodes;

        for (int i = 0; i < alloc.size(); ++i) {
            const auto frag = alloc.get_fragment(i);
            nodes.emplace_back (get_data_node (frag.pointer));
            node_address_map [nodes.back()].push_fragment(frag);
        }

        std::vector <address> data_pieces;
        data_pieces.reserve(nodes.size());

        for (const auto& item: node_address_map) {
            data_pieces.emplace_back(item.second);
        }

        auto bc = [&data_pieces] (auto m, auto id) -> coro <message_type> {
            co_await m.get ().send_address (DEALLOC_REQ, data_pieces.at(id));
            const auto h = co_await m.get ().recv_header ();
            co_return h.type;
        };
        const auto resp = co_await broadcast_gather_custom <message_type> (*m_io_service, nodes, bc);
        co_return;
    }


    coro <void> scatter_allocated_write (const address& addr, const std::string_view& data) {

        if (data.size() % m_data_node_offsets.size() != 0) [[unlikely]] {
            throw std::length_error ("Invalid data length for broadcast");
        }

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::vector <std::shared_ptr <client>> nodes;

        for (int i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            auto n = get_data_node (frag.pointer);
            auto& node_address = node_address_map [n];
            if (node_address.empty()) {
                nodes.emplace_back(std::move (n));
            }
            node_address.push_fragment(frag);
        }

        const auto part_size = data.size() / m_data_node_offsets.size();
        const auto& ec_nodes = m_ec->get_ec_nodes();
        const auto ec_data = m_ec->compute_ec(data, static_cast <int> (m_data_node_offsets.size()));

        auto bc_func = [&] (auto m, int node_id) -> coro <int> {
            std::string_view data_part;
            const auto node = nodes.at (node_id);
            if (std::find (ec_nodes.cbegin(), ec_nodes.cend(), node) == ec_nodes.cend()) { // no ec node
                data_part = data.substr(node_id * part_size, part_size);
            }
            else { // ec node
                const auto index = m_data_node_offsets.size() - node_id;
                data_part = std::string_view {ec_data [index].data.get(), ec_data[index].size};
            }
            allocated_write_message msg {.addr = node_address_map.at (node),
                                         .data = data_part};
            co_await m.get ().send_allocated_write (ALLOC_WRITE_REQ, msg);
            co_await m.get ().recv_header ();
            co_return node_id;
        };

        co_await broadcast_gather_custom <int> (*m_io_service, nodes, bc_func);

        co_return;
    }


    coro <address> scatter_write (const std::string_view& data) {

        if (data.size() % m_data_node_offsets.size() != 0) [[unlikely]] {
            throw std::length_error ("Invalid data length for broadcast");
        }

        const auto& ec_nodes = m_ec->get_ec_nodes();

        std::vector <std::shared_ptr <client>> nodes;
        nodes.reserve(m_data_node_offsets.size() + ec_nodes.size());
        for (const auto& n: m_data_node_offsets) {
            nodes.emplace_back(n.second);
        }
        for (const auto& n: ec_nodes) {
            nodes.emplace_back(n);
        }

        const auto part_size = data.size() / m_data_node_offsets.size();
        const auto ec_data = m_ec->compute_ec(data, static_cast <int> (m_data_node_offsets.size()));

        std::vector <address> addrs (nodes.size());
        auto bc_func = [&] (auto m, int node_id) -> coro <int> {
            std::string_view data_part;
            const auto node = nodes.at (node_id);
            if (std::find (ec_nodes.cbegin(), ec_nodes.cend(), node) == ec_nodes.cend()) { // no ec node
                data_part = data.substr(node_id * part_size, part_size);
            }
            else { // ec node
                const auto index = m_data_node_offsets.size() - node_id;
                data_part = std::string_view {ec_data [index].data.get(), ec_data[index].size};
            }
            co_await m.get ().send (WRITE_REQ, data_part);
            const auto message_header = co_await m.get().recv_header();
            auto resp = co_await m.get().recv_address(message_header);
            addrs[node_id] = std::move (resp);
            co_return node_id;
        };

        co_await broadcast_gather_custom <int> (*m_io_service, nodes, bc_func);

        address addr;
        for (const auto& a: addrs) {
            addr.append_address(a);
        }

        sspan <char> buf (addr.sizes[0]);
        std::memcpy (buf.data(), data.data(), addr.sizes[0]);
        m_cache_l2.put (addr.get_fragment(0).pointer, std::move (buf));

        co_return std::move (addr);
    }

    coro <void> recover () {
        std::vector <std::shared_ptr <client>> nodes;
        nodes.reserve(m_data_node_offsets.size() + m_ec->get_acquired_ec_node_count());
        std::vector <uint128_t> offsets;
        offsets.reserve(m_data_node_offsets.size() + m_ec->get_acquired_ec_node_count());
        for (auto& node: m_data_node_offsets) {
            offsets.emplace_back(node.first);
            nodes.emplace_back(node.second);
        }
        for (auto& node: m_ec->get_ec_node_offset_map()) {
            offsets.emplace_back(node.first);
            nodes.emplace_back(node.second);
        }

        auto bc_func = [] (auto m, int node_id) -> coro <uint128_t> {
            co_await m.get ().send(USED_REQ, {});
            const auto message_header = co_await m.get().recv_header();
            const auto resp = co_await m.get().recv_uint128_t (message_header);
            co_return resp;

        };
        const auto resp = co_await broadcast_gather_custom <uint128_t> (*m_io_service, nodes, bc_func);

        // assert that only one node has 0 used and the rest have equal positive value

        std::map <uint128_t, std::vector <std::shared_ptr <client>>> node_sizes;
        std::map <uint128_t, std::vector <uint128_t>> offset_sizes;

        for (const auto r: resp) {
            node_sizes [r.second].emplace_back (nodes.at(r.first));
            offset_sizes [r.second].emplace_back (offsets.at(r.first));
        }

        if (node_sizes.size() == 1) [[likely]] {
            co_return;
        }
        else if (node_sizes.cbegin()->first != data_store::alloc_offset) { // we need to delete the existing data of the lost node first
            throw std::runtime_error ("Complicated recovery mechanism not supported yet!");
        }
        else if (node_sizes.size() > 2 or node_sizes.cbegin()->second.size() > m_ec->get_maximum_allowed_failed_nodes_count()) {
            throw std::runtime_error ("More failed nodes than EC can tolerate!");
        }

        const auto& failed_nodes = node_sizes.cbegin()->second;
        const auto& healthy_nodes = std::next (node_sizes.begin())->second;
        const auto& healthy_offsets = std::next (offset_sizes.begin())->second;
        const auto& data_size = std::next (node_sizes.begin())->first;

        const auto chunk_size = m_cluster_map.m_cluster_conf.global_data_view_conf.recovery_chunk_size;
        const auto max_file_size = m_cluster_map.m_cluster_conf.data_node_conf.max_file_size;
        uint128_t last_file_end_offset = max_file_size;
        uint128_t offset = data_store::alloc_offset;
        while (offset < data_size) {
            const auto min_end = std::min (data_size, last_file_end_offset);
            while (offset < min_end) {
                const auto adjusted_chunk_size = std::min (uint128_t (chunk_size), (min_end - offset)).get_low();

                auto read_bc =  [&] (auto m, int node_id) -> coro <ospan <char>> {
                    ospan <char> buf (adjusted_chunk_size);
                    co_await m.get().send_fragment(READ_REQ, {offset + healthy_offsets.at (node_id), buf.size});
                    co_await m.get().recv ({buf.data.get(), buf.size});
                    co_return std::move (buf);
                };
                const auto response = co_await broadcast_gather_custom <ospan <char>> (*m_io_service, healthy_nodes, read_bc);
                const auto recovered = m_ec->recover(response, static_cast <int> (failed_nodes.size()));

                auto write_bc =  [&recovered] (auto m, int node_id) -> coro <address> {
                    const auto& data = recovered.at(node_id);
                    std::span <char> sp {data.data.get(), data.size};
                    co_await m.get ().send (WRITE_REQ, sp);
                    const auto message_header = co_await m.get().recv_header();
                    auto resp = co_await m.get().recv_address(message_header);
                    co_return resp;
                };
                co_await broadcast_gather_custom <address> (*m_io_service, failed_nodes, write_bc);

                offset += adjusted_chunk_size;
            }
            if (min_end < data_size) {
                last_file_end_offset += max_file_size;
                offset += data_store::alloc_offset;
            }
        }

    }
*/
    std::size_t read_address (char* buffer, const address& addr) {

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::unordered_map <std::shared_ptr <client>, std::vector <size_t>> node_data_offsets_map;
        std::vector <std::shared_ptr <client>> nodes;

        size_t offset = 0;
        for (int i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            auto n = get_data_node (frag.pointer);
            auto& node_address = node_address_map [n];
            if (node_address.empty()) {
                nodes.emplace_back(n);
            }
            node_address.push_fragment(frag);
            node_data_offsets_map [n].emplace_back(offset);
            offset += frag.size;
        }

        std::vector <std::future <void>> futures;
        futures.reserve(node_address_map.size());

        for (auto& n: node_address_map) {
            const auto node = n.first;
            const auto& add = node_address_map.at(node);
            const auto& offsets = node_data_offsets_map.at(node);
            auto m = n.first->acquire_messenger();

            futures.emplace_back (boost::asio::co_spawn(*m_io_service, [buffer, offsets, add] (client::acquired_messenger m) -> coro <void> {
                co_await m.get ().send_address (READ_ADDRESS_REQ, add);
                const auto h = co_await m.get ().recv_header ();
                for (int i = 0; i < add.size(); ++i) {
                    m.get ().register_read_buffer (buffer + offsets.at(i), add.sizes[i]);
                }
                co_await m.get ().recv_buffers (h);
            } (std::move (m)), boost::asio::use_future));
        }

        for (auto& f: futures) {
            f.wait();
        }

        return offset;
    }

    coro <void> remove (const uint128_t pointer, const size_t size) {
        auto m = get_data_node (pointer)->acquire_messenger();
        co_await m.get().send_fragment(REMOVE_REQ, {pointer, size});
        const auto h = co_await m.get().recv_header();
    }

    void sync (const address& addr) {

        if (addr.empty()) [[unlikely]] {
            throw std::length_error ("Empty address is not allowed for sync");
        }

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;

        for (int i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            auto n = get_data_node (frag.pointer);
            auto& node_address = node_address_map [n];
            node_address.push_fragment(frag);
        }

        std::vector <std::future <void>> futures;
        futures.reserve(node_address_map.size());

        for (auto& n: node_address_map) {
            auto m = n.first->acquire_messenger();
            futures.emplace_back (boost::asio::co_spawn (*m_io_service, [] (client::acquired_messenger m, address addr) -> coro <void> {
                co_await m.get().send_address(SYNC_REQ, addr);
                co_await m.get().recv_header();
                } (std::move (m), n.second), boost::asio::use_future));
        }

        for (auto& f: futures) {
            f.wait();
        }
    }

    [[nodiscard]] uint128_t get_used_space () {

        std::vector <std::future <uint128_t>> futures;
        futures.reserve(m_data_node_offsets.size());
        for (const auto& n: m_data_node_offsets) {
            auto m = n.second->acquire_messenger();
            futures.emplace_back (boost::asio::co_spawn (*m_io_service, [] (client::acquired_messenger m) -> coro <uint128_t> {
                co_await m.get().send(USED_REQ, {});
                const auto message_header = co_await m.get().recv_header();
                const auto resp = co_await m.get().recv_uint128_t (message_header);
                co_return resp;
            } (std::move (m)), boost::asio::use_future));
        }

        uint128_t used = 0;
        for (auto& f: futures) {
            used += f.get();
        }

        return used;
    }

    [[nodiscard]] std::size_t get_data_node_count() {
        return m_data_node_offsets.size();
    }

    void stop () {

        for (const auto& n: m_data_node_offsets) {
            auto m = n.second->acquire_messenger();
            boost::asio::co_spawn (*m_io_service, [] (client::acquired_messenger m) -> coro <uint128_t> {
                co_await m.get ().send(STOP, {});
            } (std::move (m)), boost::asio::detached);
        }
    }


    void create_data_node_connections (const std::shared_ptr <boost::asio::io_context>& io_service, int connection_count, const bool use_id_as_port_offset) {

        m_io_service = io_service;

        if (m_cluster_map.m_roles.at(DATA_NODE).size() < m_ec->get_minimum_node_count()) [[unlikely]] {
            throw std::logic_error ("The count of data nodes does not satisfy the minimum EC requirement");
        }

        int i = 0;

        for (const auto& data_node: m_cluster_map.m_roles.at(DATA_NODE)) {
            uint16_t port = m_cluster_map.m_cluster_conf.data_node_conf.server_conf.port;
            if(use_id_as_port_offset) {
                port += data_node.first;
            }
            auto cl = std::make_shared <client> (m_io_service, data_node.second, port, connection_count);
            const uint128_t offset = m_cluster_map.m_cluster_conf.data_node_conf.max_data_store_size * (data_node.first - m_ec->get_acquired_ec_node_count());

            if ( m_cluster_map.m_roles.at(DATA_NODE).size() - i <= m_ec->get_required_ec_node_count()) {
                m_ec->add_ec_node(offset, std::move (cl));
            }
            else {
                m_data_node_offsets.emplace(offset, std::move(cl));
            }
            i++;
        }

    }

    [[nodiscard]] std::shared_ptr <boost::asio::io_context> get_executor () const {
        return m_io_service;
    }

    [[nodiscard]] inline std::size_t l1_cache_sample_size () const noexcept {
        return m_cluster_map.m_cluster_conf.global_data_view_conf.l1_sample_size;
    }

private:

    std::shared_ptr <client> get_data_node (const uint128_t& pointer) {
        const auto pfd = m_data_node_offsets.upper_bound (pointer);

       if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
            throw std::out_of_range ("The pointer is not in the range of data nodes");
       }
       const auto n = std::prev (pfd);
       if (pfd == m_data_node_offsets.cend() and n->first + m_cluster_map.m_cluster_conf.data_node_conf.max_data_store_size < pointer) {
            return m_ec->get_ec_node (pointer);
        }
        return n->second;
    }

    const cluster_map& m_cluster_map;
    std::shared_ptr <boost::asio::io_context> m_io_service;
    std::map <const uint128_t, std::shared_ptr <client>> m_data_node_offsets;
    std::unique_ptr <ec> m_ec;
    std::atomic <size_t> m_data_node_index {};
    lru_cache <uint128_t, sspan <char>> m_cache_l1;
    lru_cache <uint128_t, sspan <char>> m_cache_l2;

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_VIEW_H
