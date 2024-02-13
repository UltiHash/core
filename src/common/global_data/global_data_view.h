#ifndef CORE_GLOBAL_DATA_VIEW_H
#define CORE_GLOBAL_DATA_VIEW_H

#include <map>
#include "common/network/client.h"
#include "lru_cache.h"
#include "common/utils/cluster_config.h"
#include "common/utils/worker_utils.h"
#include "common/types/shared_buffer.h"
#include "common/registry/services.h"

namespace uh::cluster {

class global_data_view {

public:

    explicit global_data_view (const global_data_view_config& config, boost::asio::io_context& ioc, services<STORAGE_SERVICE>& storage_services):
            m_io_service (ioc),
            m_storage_services(storage_services),
            m_config(config),
            m_cache_l1 (m_config.read_cache_capacity_l1),
            m_cache_l2 (m_config.read_cache_capacity_l2)
    {
        m_storage_services.get();
    }

    address write (const std::string_view& data) {

        const auto client = m_storage_services.get();

        address addr;
        boost::asio::co_spawn(m_io_service, [&data, &addr] (client::acquired_messenger m)-> coro <void> {
                co_await m.get().send(WRITE_REQ, data);
                const auto message_header = co_await m.get().recv_header();
                addr = co_await m.get().recv_address(message_header);
            } (client->acquire_messenger()), boost::asio::use_future).get();

        shared_buffer <char> l1_buf (std::min (addr.first().size, m_config.l1_sample_size));
        std::memcpy (l1_buf.data(), data.data(), l1_buf.size());
        m_cache_l1.put (addr.first().pointer, std::move (l1_buf));
        return addr;
    }

    shared_buffer <char> read_l1_cache (const uint128_t pointer, const size_t size) {
        if (const auto c = m_cache_l1.get(pointer, nullptr); c.data() != nullptr) {
            if (c.size() >= size) [[likely]] {
                return c;
            }
        }
        return nullptr;
    }

    shared_buffer <char> read_l2_cache (const uint128_t pointer, const size_t size) {
        if (const auto c = m_cache_l2.get(pointer, nullptr); c.data() != nullptr) {
            if (c.size() >= size) [[likely]] {
                return c;
            }
        }
        return nullptr;
    }

    size_t read (char* buffer, const uint128_t pointer, const size_t size) {
        const fragment frag {pointer, size};
        size_t read_size = 0;
        boost::asio::co_spawn(m_io_service, [&frag, &buffer, &read_size] (client::acquired_messenger m)-> coro <void> {
            co_await m.get().send_fragment(READ_REQ, frag);
            const auto h = co_await m.get().recv_header();
            read_size = h.size;
            m.get().register_read_buffer (buffer, read_size);
            co_await m.get().recv_buffers(h);
        } (m_storage_services.get(pointer)->acquire_messenger()), boost::asio::use_future).get();

        // l1 cache
        shared_buffer <char> l1_buf (std::min (read_size, m_config.l1_sample_size));
        std::memcpy (l1_buf.data(), buffer, l1_buf.size());
        m_cache_l2.put (pointer, std::move (l1_buf));

        // l2 cache
        shared_buffer <char> l2_buf (read_size);
        std::memcpy (l2_buf.data(), buffer, read_size);
        m_cache_l2.put (pointer, std::move (l2_buf));

        return read_size;

    }

    std::size_t read_address (char* buffer, const address& addr) {

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::unordered_map <std::shared_ptr <client>, std::vector <size_t>> node_data_offsets_map;
        std::vector <std::shared_ptr <client>> nodes;

        size_t offset = 0;
        for (size_t i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            auto n = m_storage_services.get (frag.pointer);
            auto& node_address = node_address_map [n];
            if (node_address.empty()) {
                nodes.emplace_back(n);
            }
            node_address.push_fragment(frag);
            node_data_offsets_map [n].emplace_back(offset);
            offset += frag.size;
        }

        worker_utils::broadcast_from_worker_in_io_threads (nodes, m_io_service, [&buffer, &nodes, &node_address_map, &node_data_offsets_map] (client::acquired_messenger m, long id) -> coro <void> {
            const auto node = nodes.at(id);
            const auto& add = node_address_map.at(node);
            const auto& offsets = node_data_offsets_map.at(node);
            co_await m.get ().send_address (READ_ADDRESS_REQ, add);
            const auto h = co_await m.get ().recv_header ();
            m.get().reserve_read_buffers(add.size());
            for (size_t i = 0; i < add.size(); ++i) {
                m.get ().register_read_buffer (buffer + offsets.at(i), add.sizes[i]);
            }
            co_await m.get ().recv_buffers (h);
        });

        return offset;
    }

    coro <void> remove (const uint128_t pointer, const size_t size) {
        auto m = m_storage_services.get(pointer)->acquire_messenger();
        co_await m.get().send_fragment(REMOVE_REQ, {pointer, size});
        co_await m.get().recv_header();
    }

    void sync (const address& addr) {

        if (addr.empty()) [[unlikely]] {
            throw std::length_error ("Empty address is not allowed for sync");
        }

        std::unordered_map <std::shared_ptr <client>, address> node_address_map;
        std::vector <std::shared_ptr <client>> nodes;

        for (size_t i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get_fragment(i);
            auto n = m_storage_services.get(frag.pointer);
            auto& node_address = node_address_map [n];
            if (node_address.empty()) {
                nodes.emplace_back(std::move (n));
            }
            node_address.push_fragment(frag);
        }

        worker_utils::broadcast_from_worker_in_io_threads (nodes, m_io_service, [&nodes, &node_address_map] (client::acquired_messenger m, long id) -> coro <void> {
            co_await m.get().send_address(SYNC_REQ, node_address_map.at(nodes [id]));
            co_await m.get().recv_header();
        });
    }

    [[nodiscard]] uint128_t get_used_space () {

        auto nodes = m_storage_services.get_clients();

        std::vector <uint128_t> used_spaces (nodes.size());

        worker_utils::broadcast_from_worker_in_io_threads (nodes, m_io_service, [&used_spaces] (client::acquired_messenger m, long id) -> coro <void> {
            co_await m.get().send(USED_REQ, {});
            const auto message_header = co_await m.get().recv_header();
            used_spaces [id] = co_await m.get().recv_uint128_t (message_header);
        });

        uint128_t used = std::accumulate(used_spaces.cbegin(), used_spaces.cend(), uint128_t {0});

        return used;
    }

    [[nodiscard]] boost::asio::io_context& get_executor () const {
        return m_io_service;
    }

    [[nodiscard]] inline std::size_t l1_cache_sample_size () const noexcept {
        return m_config.l1_sample_size;
    }

    [[nodiscard]] inline std::size_t get_storage_service_connection_count() const noexcept {
        return m_config.storage_service_connection_count;
    }

private:

    boost::asio::io_context& m_io_service;

    services<STORAGE_SERVICE>& m_storage_services;
    global_data_view_config m_config;

    lru_cache <uint128_t, shared_buffer <char>> m_cache_l1;
    lru_cache <uint128_t, shared_buffer <char>> m_cache_l2;

};

} // end namespace uh::cluster

#endif //CORE_GLOBAL_DATA_VIEW_H
