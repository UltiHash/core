//
// Created by masi on 8/29/23.
//

#ifndef CORE_DATA_NODE_HANDLER_H
#define CORE_DATA_NODE_HANDLER_H

#include <utility>
#include "common/common.h"
#include "common/protocol_handler.h"
#include "data_store.h"

namespace uh::cluster {

class data_node_handler: public protocol_handler {
public:

    data_node_handler(data_node_config conf, int id) :
            protocol_handler(conf.server_conf),
            m_data_store(conf, id),
            m_is_stopped(false),

            m_req_counters(add_counter_family("uh_dn_requests", "number of requests handled by the data node")),
            m_reqs_write(m_req_counters.Add({{"message_type", "WRITE_REQ"}})),
            m_reqs_read(m_req_counters.Add({{"message_type", "READ_REQ"}})),
            m_reqs_read_address(m_req_counters.Add({{"message_type", "READ_ADDRESS_REQ"}})),
            m_reqs_remove(m_req_counters.Add({{"message_type", "REMOVE_REQ"}})),
            m_reqs_sync(m_req_counters.Add({{"message_type", "SYNC_REQ"}})),
            m_reqs_used(m_req_counters.Add({{"message_type", "USED_REQ"}})),
            m_reqs_alloc(m_req_counters.Add({{"message_type", "ALLOC_REQ"}})),
            m_reqs_dealloc(m_req_counters.Add({{"message_type", "DEALLOC_REQ"}})),
            m_reqs_alloc_write(m_req_counters.Add({{"message_type", "ALLOC_WRITE_REQ"}})),
            m_reqs_query_usage(m_req_counters.Add({{"message_type", "QUERY_USAGE_REQ"}})),
            m_reqs_invalid(m_req_counters.Add({{"message_type", "INVALID"}})),

            m_resource_util(add_gauge_family("uh_dn_resource_utilization", "resources utilization of the data node instance")),
            m_util_used_storage(m_resource_util.Add({{"resource_type", "used_storage"}})),
            m_util_free_storage(m_resource_util.Add({{"resource_type", "free_storage"}})),

            m_configs(add_gauge_family("uh_dn_config_parameters", "configuration parameters the data node instance has been set-up with")),
            m_config_min_file_size(m_configs.Add({{"config_parameter", "min_file_size"}})),
            m_config_max_file_size(m_configs.Add({{"config_parameter", "max_file_size"}})),
            m_config_max_data_store_size(m_configs.Add({{"config_parameter", "max_data_store_size"}}))


    {
        init();
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        m_config_min_file_size.Set(conf.min_file_size);
        m_config_max_file_size.Set(conf.max_file_size);
        m_config_max_data_store_size.Set(conf.max_data_store_size.get_low());
    }

    coro <void> handle (messenger m) override {
        for (;;) {
            const auto message_header = co_await m.recv_header();
            switch (message_header.type) {
                case WRITE_REQ:
                    m_reqs_write.Increment();
                    co_await handle_write(m, message_header);
                    break;
                case READ_REQ:
                    m_reqs_read.Increment();
                    co_await handle_read(m, message_header);
                    break;
                case READ_ADDRESS_REQ:
                    m_reqs_read_address.Increment();
                    co_await handle_read_address (m, message_header);
                    break;
                case REMOVE_REQ:
                    m_reqs_remove.Increment();
                    co_await handle_remove(m, message_header);
                    break;
                case SYNC_REQ:
                    m_reqs_sync.Increment();
                    co_await handle_sync(m, message_header);
                    break;
                case USED_REQ:
                    m_reqs_used.Increment();
                    co_await handle_get_used(m, message_header);
                    break;
                case ALLOC_REQ:
                    m_reqs_alloc.Increment();
                    co_await handle_alloc (m, message_header);
                    break;
                case DEALLOC_REQ:
                    m_reqs_dealloc.Increment();
                    co_await handle_dealloc (m, message_header);
                    break;
                case ALLOC_WRITE_REQ:
                    m_reqs_alloc_write.Increment();
                    co_await handle_alloc_write (m, message_header);
                    break;
                case STOP:
                    m_is_stopped = true;
                    co_return;
                default:
                    m_reqs_invalid.Increment();
                    throw std::invalid_argument("Invalid message type!");
            }
        }
    }

    bool stop_received() const override {
        return m_is_stopped;
    }

private:

    coro <void> handle_write (messenger &m, const messenger::header& h) {
        ospan <char> data (h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);
        const auto addr = m_data_store.write({data.data.get(), data.size});
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        co_await m.send_address(WRITE_RESP, addr);
    }

    coro <void> handle_read (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        ospan <char> buffer (resp.second.size);
        const auto size = m_data_store.read(buffer.data.get(), resp.second.pointer, resp.second.size);
        co_await m.send (READ_RESP, {buffer.data.get(), size});
    }

    coro <void> handle_read_address (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_address(h);
        const auto read_size = std::accumulate (resp.second.sizes.cbegin(), resp.second.sizes.cend(), 0);
        ospan <char> buffer (read_size);
        size_t offset = 0;
        for (int i = 0; i < resp.second.size(); i++) {
            const auto frag = resp.second.get_fragment(i);
            if (m_data_store.read(buffer.data.get() + offset, frag.pointer, frag.size) != frag.size) [[unlikely]] {
                throw std::runtime_error ("Could not read the data with the given size");
            }
            offset += frag.size;
        }
        co_await m.send (READ_ADDRESS_RESP, {buffer.data.get(), offset});
    }

    coro <void> handle_remove (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        m_data_store.remove(resp.second.pointer, resp.second.size);
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        co_await m.send (REMOVE_OK, {});
    }

    coro <void> handle_sync (messenger &m, const messenger::header& h) {
        co_await m.recv_address(h);
        m_data_store.sync();
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        co_await m.send (SYNC_OK, {});
    }

    coro <void> handle_get_used (messenger &m, const messenger::header& h) {
        const auto used = m_data_store.get_used_space();
        co_await m.send_uint128_t(USED_RESP, used);
    }

    coro <void> handle_alloc (messenger &m, const messenger::header& h) {
        size_t size;
        m.register_read_buffer(size);
        //std::cout << "data node handle alloc" << std::endl;
        co_await m.recv_buffers(h);
        //std::cout << "data node handle alloc recv size " << size << std::endl;
        const auto addr = m_data_store.allocate(size);
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        //std::cout << "data node handle alloc after alloc for size " << size << std::endl;
        co_await m.send_address(ALLOC_RESP, addr);
        //std::cout << "data node handle alloc after send alloc size " << size << std::endl;
    }

    coro <void> handle_dealloc (messenger &m, const messenger::header& h) {
        std::vector <char> data (h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);
        address addr;
        zpp::bits::in {data, zpp::bits::size4b{}} (addr).or_throw();
        m_data_store.cancel_allocate(addr);
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        co_await m.send(DEALLOC_RESP, {});
    }

    coro <void> handle_alloc_write (messenger &m, const messenger::header& h) {
        const auto msg = co_await m.recv_allocated_write(h);
        const auto& data_ospan = std::get <ospan <char>> (msg.data);
        m_data_store.allocated_write(msg.addr, {data_ospan.data.get(), data_ospan.size});
        m_util_used_storage.Set(m_data_store.get_used_space().get_low());
        m_util_free_storage.Set(m_data_store.get_free_space().get_low());
        co_await m.send(ALLOC_WRITE_RESP, {});
    }

    uh::cluster::data_store m_data_store;
    bool m_is_stopped;

    prometheus::Family<prometheus::Counter>& m_req_counters;
    prometheus::Counter& m_reqs_write;
    prometheus::Counter &m_reqs_read;
    prometheus::Counter &m_reqs_read_address;
    prometheus::Counter &m_reqs_remove;
    prometheus::Counter &m_reqs_sync;
    prometheus::Counter &m_reqs_used;
    prometheus::Counter &m_reqs_alloc;
    prometheus::Counter &m_reqs_dealloc;
    prometheus::Counter &m_reqs_alloc_write;
    prometheus::Counter &m_reqs_query_usage;
    prometheus::Counter &m_reqs_invalid;

    prometheus::Family<prometheus::Gauge> &m_resource_util;
    prometheus::Gauge &m_util_used_storage;
    prometheus::Gauge &m_util_free_storage;

    prometheus::Family<prometheus::Gauge> &m_configs;
    prometheus::Gauge &m_config_min_file_size;
    prometheus::Gauge &m_config_max_file_size;
    prometheus::Gauge &m_config_max_data_store_size;
};

} // end namespace uh::cluster

#endif //CORE_DATA_NODE_HANDLER_H
