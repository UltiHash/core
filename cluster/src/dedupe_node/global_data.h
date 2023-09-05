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


namespace uh::cluster {


class global_data {


public:

    explicit global_data (const cluster_map& cmap, int data_node_connection_count, int running_threads):
                          m_temp_offset (temp_offset_start),
                          m_cluster_map (cmap),
                          m_io_service (running_threads) {
        sleep(2);
        create_data_node_connections(data_node_connection_count);
    }

    /*

address cache_write (const std::string_view& data) {
    m_temp_offset = m_temp_offset - 1;
    m_cache.emplace(m_temp_offset, data);
    address addr;
    addr.push_fragment(fragment {m_temp_offset, data.size()});
    return addr;
}

std::unordered_map <uint128_t, address> sync_cache () {
    return {};
    if (m_temp_offset == temp_offset_start) [[unlikely]] {
        return {};
    }

    std::vector <MPI_Request> reqs (m_data_nodes.size());
    std::vector <MPI_Status> stats (m_data_nodes.size());

    const auto total_size = (temp_offset_start - m_temp_offset).get_data()[1];
    auto buf = std::make_unique_for_overwrite <char[]> (total_size);

    std::size_t offset = 0;
    for (const auto& cached: m_cache) {
        std::memcpy(buf.get() + offset, cached.second.data(), cached.second.size());
        offset += cached.second.size();
    }

    const int portion = static_cast <int> (total_size / m_data_nodes.size());
    int rc;
    for (int i = 0; i < m_data_nodes.size(); ++i) {
        rc = MPI_Isend (buf.get() + i * portion, portion, MPI_CHAR, m_data_nodes[i], message_types::WRITE_REQ, MPI_COMM_WORLD, &reqs[i]);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
    }

    MPI_Waitall(static_cast <int> (m_data_nodes.size()), reqs.data(), stats.data());

    size_t total_response_size = 0;
    std::vector <int> message_sizes (m_data_nodes.size());
    for (int i = 0; i < m_data_nodes.size(); ++i) {
        rc = MPI_Probe(MPI_ANY_SOURCE, message_types::WRITE_RESP, MPI_COMM_WORLD, &stats[i]);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }

        MPI_Get_count(&stats[i], MPI_CHAR, &message_sizes[i]);
        total_response_size += message_sizes[i];
    }

    address addresses (total_response_size);

    offset = 0;
    for (int i = 0; i < m_data_nodes.size(); ++i) {
        rc = MPI_Irecv (reinterpret_cast <char *> (addresses.data()) + offset, portion, MPI_CHAR, m_data_nodes[i], message_types::WRITE_RESP, MPI_COMM_WORLD, &reqs[i]);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            MPI_Abort(MPI_COMM_WORLD, rc);
        }
        offset += message_sizes[i];
    }

    MPI_Waitall(static_cast <int> (m_data_nodes.size()), reqs.data(), stats.data());

    std::unordered_map <uint128_t, address> result;
    std::size_t i = 0;
    for (const auto& cached: m_cache) {
        std::size_t size = 0;
        address addr;
        while (size < cached.second.size()) {
            addr.emplace_back(addresses[i]);
            size += addresses[i].size;
        }
        if (size != cached.second.size()) [[unlikely]] {
            throw std::logic_error ("Size mismatch when receiving the address of the written data");
        }
        result.emplace(cached.first, std::move (addr));
    }

    m_cache.clear();
    m_temp_offset = temp_offset_start;

    return result;
    }
     */


    coro <address> write (const std::string_view& data) {
        auto m = m_data_node_offsets.at(0).acquire_messenger();
        co_await m.get().send(WRITE_REQ, data);
        const auto message_header = co_await m.get().recv_header();
        auto resp = co_await m.get().recv_address(message_header);
        if (resp.first.type != WRITE_RESP) [[unlikely]] {
            throw std::runtime_error ("Invalid response for write request");
        }
        co_return std::move (resp.second);
    }

    coro <std::size_t> read (char* buffer, const uint128_t pointer, const size_t size) {
        auto m = get_data_node (pointer).acquire_messenger();
        co_await m.get().send_fragment(READ_REQ, {pointer, size});
        const auto h = co_await m.get().recv ({buffer, size});
        if (h.type != READ_RESP) [[unlikely]] {
            throw std::runtime_error ("Read not successful");
        }
        co_return h.size;
    }

    coro <void> remove (const uint128_t pointer, const size_t size) {
        auto m = get_data_node (pointer).acquire_messenger();
        co_await m.get().send_fragment(REMOVE_REQ, {pointer, size});
        const auto h = co_await m.get().recv_header();
        if (h.type != REMOVE_OK) [[unlikely]] {
            throw std::runtime_error ("Remove not successful");
        }
    }

    coro <void> sync (const address& addr) {

        std::unordered_map <client*, address> dn_address_map;
        for (int i = 0; i < addr.size(); i++) {
            const auto frag = addr.get_fragment(i);
            auto& cl = get_data_node (frag.pointer);
            if (const auto f = dn_address_map.find(&cl); f != dn_address_map.cend()) {
                f->second.push_fragment(frag);
            }
            else {
                dn_address_map.emplace (&cl, frag);
            }
        }

        bool success = true;
        for (const auto& dn_address: dn_address_map) {

            auto m = dn_address.first->acquire_messenger();
            co_await m.get ().send_address(REMOVE_REQ, dn_address.second);
            const auto h = co_await m.get().recv_header();
            success = success and (h.type == REMOVE_OK);
        }

        if (!success) [[unlikely]] {
            throw std::runtime_error ("Remove not successful");
        }
    }

    [[nodiscard]] coro <uint128_t> get_used_space () {

        std::forward_list <client::acquired_messenger> messengers;
        for (auto& dn: m_data_node_offsets) {
            messengers.emplace_front(dn.second.acquire_messenger());
            co_await messengers.front().get().send(USED_REQ, {});
        }

        bool success = true;
        uint128_t used = 0;
        for (auto& m: messengers) {
            const auto message_header = co_await m.get().recv_header();
            const auto resp = co_await m.get().recv_uint128_t(message_header);
            success = success and (resp.first.type == USED_RESP);
            used = used + resp.second;
        }

        if (!success) [[unlikely]] {
            throw std::runtime_error ("Get used space not successful");
        }
        co_return used;
    }

    coro <void> stop () {
        for (auto& dn: m_data_node_offsets) {
            auto m = dn.second.acquire_messenger();
            co_await m.get().send(STOP, {});
        }
    }

    uint128_t m_temp_offset;

private:

    void create_data_node_connections (int connection_count) {

        for (const auto& data_node: m_cluster_map.m_roles.at(DATA_NODE)) {
            const uint128_t offset = m_cluster_map.m_cluster_conf.data_node_conf.max_data_store_size * data_node.first;
            auto cl = client (m_io_service, data_node.second,
                              m_cluster_map.m_cluster_conf.data_node_conf.server_conf.port, connection_count);
            m_data_node_offsets.emplace(offset, std::move (cl));
        }
    }

   client& get_data_node (const uint128_t& pointer) {
        const auto pfd = m_data_node_offsets.upper_bound (pointer);
        if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
            throw std::out_of_range ("The given data pointer could not be found among the available data nodes");
        }
        return std::prev (pfd)->second;
    }

    const cluster_map& m_cluster_map;
    boost::asio::io_context m_io_service;
    std::unordered_map <uint128_t, std::string_view> m_cache;
    std::map <const uint128_t, client> m_data_node_offsets;
    std::vector <int> m_data_nodes;

    constexpr static uint128_t temp_offset_start = uint128_t (std::numeric_limits <uint64_t>::max(), std::numeric_limits <uint64_t>::max());

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_H
