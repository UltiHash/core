//
// Created by masi on 7/27/23.
//

#ifndef CORE_GLOBAL_DATA_H
#define CORE_GLOBAL_DATA_H

#include <map>
#include <unordered_map>
#include <set>
#include "data_node/data_node.h"
#include "network/client.h"
#include <network/network_traits.h>

namespace uh::cluster {

using namespace std::placeholders;

class global_data {


public:

    explicit global_data (const cluster_map& cmap, int data_node_connection_count, const std::shared_ptr <boost::asio::io_context>& ioc):
                          m_cluster_map (cmap),
                          m_io_service (ioc) {
        sleep(2);
        create_data_node_connections(data_node_connection_count);
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

    coro <address> allocate (size_t size) {
        std::vector <std::shared_ptr <client>> nodes;
        for (auto& node: m_data_node_offsets) {
            nodes.emplace_back(node.second);
        }

        std::list <std::string> errors;
        address addr;
        const auto resp = broadcast_gather ({}, *m_io_service, nodes, ALLOC_REQ, std::span <char> {reinterpret_cast <char*> (&size), sizeof size});
        for (const auto& r: resp) {

            address node_addr;
            if (r.first.type != ALLOC_RESP) [[unlikely]] {
                // handle errors
            }
            zpp::bits::in {std::span <char> {r.second.data.get(), r.second.size}, zpp::bits::size4b{}} (node_addr).or_throw();
            addr.append_address(node_addr);
        }

        if (!errors.empty()) {
            // handle
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

        std::vector <std::vector <char>> data_pieces;
        data_pieces.reserve(nodes.size());

        for (const auto& item: node_address_map) {
            data_pieces.emplace_back();
            zpp::bits::out {data_pieces.back(), zpp::bits::size4b {}} (item.second).or_throw();
        }

        const auto resp = scatter_gather <std::vector <std::vector <char>>> ({}, *m_io_service, nodes, DEALLOC_REQ, data_pieces);
        for (const auto& r: resp) {
            if (r.first.type == FAILURE) [[unlikely]] {
                const auto msg = std::string_view {r.second.data.get(), r.second.size};
                std::string msgstr (msg);
                throw std::runtime_error ("Received failure by cancel allocation with message: " + msgstr);
            }
        }
    }


    coro <void> scatter_allocated_write (const address& addr, const std::string_view& data) {

        if (std::accumulate(addr.sizes.cbegin(), addr.sizes.cend(), 0ul) != data.size()) [[unlikely]] {
            throw std::bad_array_new_length ();
        }

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::vector <std::shared_ptr <client>> nodes;

        for (int i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            nodes.emplace_back (get_data_node (frag.pointer));
            node_address_map [nodes.back()].push_fragment(frag);
        }

        if (data.size() % nodes.size() != 0) [[unlikely]] {
            throw std::length_error ("Invalid data length for broadcast");
        }
        const auto part_size = data.size() / nodes.size();

        auto bc_func = [&] (auto && m, int node_id) -> coro <std::pair <messenger::header, int>> {
            allocated_write_message msg {.addr = node_address_map.at (nodes.at (node_id)),
                                         .data = data.substr(node_id * part_size, part_size)};
            co_await m.get ().send_allocated_write (ALLOC_WRITE_REQ, msg);
            const auto h = co_await m.get ().recv_header ();
            co_await m.get ().throw_if_failure (h);
            co_return std::pair <messenger::header, int> (h, node_id);
        };

        broadcast_gather_custom <int> ({}, *m_io_service, nodes, bc_func);

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

        std::unordered_map <std::shared_ptr <client>, address> dn_address_map;
        for (int i = 0; i < addr.size(); i++) {
            const auto frag = addr.get_fragment(i);
            auto cl = get_data_node (frag.pointer);
            if (const auto f = dn_address_map.find(cl); f != dn_address_map.cend()) {
                f->second.push_fragment(frag);
            }
            else {
                dn_address_map.emplace (cl, frag);
            }
        }

        bool success = true;
        for (const auto& dn_address: dn_address_map) {

            auto m = dn_address.first->acquire_messenger();
            co_await m.get ().send_address(SYNC_REQ, dn_address.second);
            const auto h = co_await m.get().recv_header();
            success = success and (h.type == SYNC_OK);
        }

        if (!success) [[unlikely]] {
            throw std::runtime_error ("Remove not successful");
        }
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
        return m_cluster_map.m_roles.at(DATA_NODE).size();
    }

    coro <void> stop () {
        for (auto& dn: m_data_node_offsets) {
            auto m = dn.second->acquire_messenger();
            co_await m.get().send(STOP, {});
        }
    }

private:

    void create_data_node_connections (int connection_count) {

        for (const auto& data_node: m_cluster_map.m_roles.at(DATA_NODE)) {
            const uint128_t offset = m_cluster_map.m_cluster_conf.data_node_conf.max_data_store_size * data_node.first;
            auto cl = std::make_shared <client> (m_io_service, data_node.second,
                              m_cluster_map.m_cluster_conf.data_node_conf.server_conf.port, connection_count);
            m_data_node_offsets.emplace(offset, std::move (cl));
        }
    }

   std::shared_ptr <client> get_data_node (const uint128_t& pointer) {
        const auto pfd = m_data_node_offsets.upper_bound (pointer);
        if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
            throw std::out_of_range ("The given data pointer could not be found among the available data nodes");
        }
        return std::prev (pfd)->second;
    }

    const cluster_map& m_cluster_map;
    std::shared_ptr <boost::asio::io_context> m_io_service;
    std::map <const uint128_t, std::shared_ptr <client>> m_data_node_offsets;
    std::atomic <size_t> m_data_node_index {};

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_H
