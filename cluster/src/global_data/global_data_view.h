//
// Created by masi on 7/27/23.
//

#ifndef CORE_GLOBAL_DATA_VIEW_H
#define CORE_GLOBAL_DATA_VIEW_H

#include <map>
#include <unordered_map>
#include <set>
#include "data_node/data_node.h"
#include "network/client.h"
#include "network/network_traits.h"
#include "ec.h"
#include "ec_factory.h"

namespace uh::cluster {

using namespace std::placeholders;

class global_data_view {


public:

    explicit global_data_view (const cluster_map& cmap):
                          m_cluster_map (cmap),
                          m_ec (ec_factory::make_ec (cmap.m_cluster_conf.ec_algorithm)){
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
        if (resp.first.type != WRITE_RESP) [[unlikely]] {
            throw std::runtime_error ("Invalid response for write request");
        }
        co_return std::move (resp.second);
    }

    coro <address> allocate (size_t total_size) {

        if (total_size % m_data_node_offsets.size() != 0) {
            throw std::length_error ("");
        }

        const auto size = total_size / m_data_node_offsets.size();

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
            co_return std::move (resp.second);
        };
        const auto resp = broadcast_gather_custom <address> (*m_io_service, nodes, bc);
        for (const auto& r: resp) {
            addr.append_address(r);
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
        const auto resp = broadcast_gather_custom <message_type> (*m_io_service, nodes, bc);
        co_return;
    }


    coro <void> scatter_allocated_write (const address& addr, const std::string_view& data) {

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::vector <std::shared_ptr <client>> nodes;

        for (int i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            nodes.emplace_back (get_data_node (frag.pointer));
            node_address_map [nodes.back()].push_fragment(frag);
        }

        if (data.size() % m_data_node_offsets.size() != 0) [[unlikely]] {
            throw std::length_error ("Invalid data length for broadcast");
        }
        const auto part_size = data.size() / m_data_node_offsets.size();
        const auto ec_nodes = m_ec->get_ec_nodes();
        std::atomic <int> ec_node_index = 0;
        const auto ec_data = m_ec->compute_ec(data, static_cast <int> (m_data_node_offsets.size()));

        auto bc_func = [&] (auto m, int node_id) -> coro <int> {
            std::string_view data_part;
            const auto node = nodes.at (node_id);
            if (std::find (ec_nodes.cbegin(), ec_nodes.cend(), node) == ec_nodes.cend()) { // no ec node
                data_part = data.substr(node_id * part_size, part_size);
            }
            else { // ec node

                auto index = ec_node_index.load();
                auto new_val = index + 1;
                while (!ec_node_index.compare_exchange_weak (index, new_val)) {
                    index = ec_node_index.load();
                    new_val = index + 1;
                }
                data_part = std::string_view {ec_data [index].data.get(), ec_data[index].size};
            }
            allocated_write_message msg {.addr = node_address_map.at (node),
                                         .data = data_part};
            co_await m.get ().send_allocated_write (ALLOC_WRITE_REQ, msg);
            const auto h = co_await m.get ().recv_header ();
            co_await m.get ().throw_if_failure (h);
            co_return node_id;
        };

        broadcast_gather_custom <int> (*m_io_service, nodes, bc_func);

        co_return;
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
            if (message_header.type == FAILURE) [[unlikely]] {
                std::string error;
                error.resize(message_header.size);
                m.get ().register_read_buffer (error);
                co_await m.get().recv_buffers(message_header);
                throw std::runtime_error ("Received failure when broadcasting get used request");
            }
            const auto resp = co_await m.get().recv_uint128_t (message_header);
            co_return resp.second;

        };
        const auto resp = broadcast_gather_custom <uint128_t> (*m_io_service, nodes, bc_func);

        // assert that only one node has 0 used and the rest have equal positive value

        std::map <uint128_t, std::vector <std::shared_ptr <client>>> node_sizes;
        std::map <uint128_t, std::vector <uint128_t>> offset_sizes;

        for (int i = 0; i < resp.size(); i++) {
            node_sizes [resp[i]].emplace_back (nodes.at(i));
            offset_sizes [resp[i]].emplace_back (offsets.at(i));
        }

        if (node_sizes.size() == 1) [[likely]] {
            co_return;
        }
        else if (node_sizes.cbegin()->first != data_store::alloc_offset) { // we need to delete the existing data of the lost node first
            throw std::runtime_error ("Complicated recovery mechanism not supported yet!");
        }
        else if (node_sizes.size() > 2 or node_sizes.cbegin()->second.size() > 1) {
            throw std::runtime_error ("More failed nodes than EC can tolerate!");
        }

        const auto& failed_nodes = node_sizes.cbegin()->second;
        const auto& healthy_nodes = std::next (node_sizes.begin())->second;
        const auto& healthy_offsets = std::next (offset_sizes.begin())->second;
        const auto& data_size = std::next (node_sizes.begin())->first;

        if (!failed_nodes.empty()) {

            const auto chunk_size = m_cluster_map.m_cluster_conf.recovery_chunk_size;
            for (uint128_t offset = data_store::alloc_offset; offset < data_size + data_store::alloc_offset; offset = offset + chunk_size) {
                auto read_bc =  [&] (auto m, int node_id) -> coro <ospan <char>> {
                    ospan <char> buf (std::min (uint128_t (chunk_size), (data_size - offset)).get_low());
                    co_await m.get().send_fragment(READ_REQ, {offset + healthy_offsets.at (node_id), buf.size});
                    const auto h = co_await m.get().recv ({buf.data.get(), buf.size});
                    if (h.type != READ_RESP) [[unlikely]] {
                        throw std::runtime_error ("Unsuccessful recovery");
                    }
                    co_return std::move (buf);
                };
                const auto response = broadcast_gather_custom <ospan <char>> (*m_io_service, healthy_nodes, read_bc);
                const auto recovered = m_ec->recover(response, static_cast <int> (failed_nodes.size()));

                auto write_bc =  [&recovered] (auto m, int node_id) -> coro <address> {
                    const auto& data = recovered.at(node_id);
                    std::span <char> sp {data.data.get(), data.size};
                    co_await m.get ().send (WRITE_REQ, sp);
                    const auto message_header = co_await m.get().recv_header();
                    auto resp = co_await m.get().recv_address(message_header);
                    if (resp.first.type != WRITE_RESP) [[unlikely]] {
                        throw std::runtime_error ("Invalid response for write request");
                    }
                    co_return resp.second;
                };
                const auto recovery_response = broadcast_gather_custom <address> (*m_io_service, failed_nodes, write_bc);
            }
        }
        co_return;
    }

    coro <std::size_t> read (char* buffer, const uint128_t pointer, const size_t size) {
        auto m = get_data_node (pointer)->acquire_messenger();
        co_await m.get().send_fragment(READ_REQ, {pointer, size});
        const auto h = co_await m.get().recv ({buffer, size});
        if (h.type != READ_RESP) [[unlikely]] {
            throw std::runtime_error ("Read not successful");
        }
        co_return h.size;
    }

    coro <void> remove (const uint128_t pointer, const size_t size) {
        auto m = get_data_node (pointer)->acquire_messenger();
        co_await m.get().send_fragment(REMOVE_REQ, {pointer, size});
        const auto h = co_await m.get().recv_header();
        if (h.type != REMOVE_OK) [[unlikely]] {
            throw std::runtime_error ("Remove not successful");
        }
    }

    coro <void> sync (const address& addr) {

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::vector <std::shared_ptr <client>> nodes;

        for (int i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            nodes.emplace_back (get_data_node (frag.pointer));
            node_address_map [nodes.back()].push_fragment(frag);
        }

        auto bc_func = [&] (auto m, int node_id) -> coro <message_type> {
            const auto node = nodes.at (node_id);

            co_await m.get ().send_address(SYNC_REQ, node_address_map.at (node));
            const auto h = co_await m.get().recv_header();
            co_return h.type;
        };

        broadcast_gather_custom <message_type> (*m_io_service, nodes, bc_func);
        // error checking

        co_return;
    }

    [[nodiscard]] coro <uint128_t> get_used_space () {

        std::forward_list <client::acquired_messenger> messengers;
        for (auto& dn: m_data_node_offsets) {

            messengers.emplace_front(dn.second->acquire_messenger());
            co_await messengers.front().get().send(USED_REQ, {});
        }

        bool success = true;
        uint128_t used = 0;
        for (auto& m: messengers) {
            const auto message_header = co_await m.get().recv_header();
            const auto resp = co_await m.get().recv_uint128_t (message_header);
            success = success and (resp.first.type == USED_RESP);
            used = used + resp.second;
        }

        if (!success) [[unlikely]] {
            throw std::runtime_error ("Get used space not successful");
        }
        co_return used;
    }

    [[nodiscard]] std::size_t get_data_node_count() {
        return m_data_node_offsets.size();
    }

    coro <void> stop () {

        std::vector <std::shared_ptr <client>> nodes;
        nodes.reserve(m_data_node_offsets.size() + m_ec->get_acquired_ec_node_count());
        for (auto& node: m_data_node_offsets) {
            nodes.emplace_back(node.second);
        }
        nodes.insert(nodes.cend(), m_ec->get_ec_nodes().cbegin(), m_ec->get_ec_nodes().cend());
        auto bc_func = [] (auto m, int node_id) -> coro <void> {
            co_await m.get ().send(STOP, {});
            co_return;
        };

        broadcast_custom (*m_io_service, nodes, bc_func);
        co_return;
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

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_VIEW_H
