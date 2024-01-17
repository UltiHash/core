//
// Created by masi on 8/29/23.
//

#ifndef CORE_DATA_STORE_SERVICE_HANDLER_H
#define CORE_DATA_STORE_SERVICE_HANDLER_H

#include <utility>
#include "common/utils/common.h"
#include "common/utils/protocol_handler.h"
#include "data_store.h"

namespace uh::cluster {

class storage_handler: public protocol_handler {
public:

    storage_handler(storage_config conf, std::size_t id) :
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
            m_reqs_invalid(m_req_counters.Add({{"message_type", "INVALID"}})),

            m_resource_util(add_gauge_family("uh_dn_resource_utilization", "resources utilization of the data node instance")),
            m_util_used_storage(m_resource_util.Add({{"resource_type", "used_storage"}})),
            m_util_free_storage(m_resource_util.Add({{"resource_type", "free_storage"}})),

            m_configs(add_gauge_family("uh_dn_config_parameters", "configuration parameters the data node instance has been set-up with")),
            m_config_min_file_size(m_configs.Add({{"config_parameter", "min_file_size"}})),
            m_config_max_file_size(m_configs.Add({{"config_parameter", "max_file_size"}})),
            m_config_max_data_store_size(m_configs.Add({{"config_parameter", "max_data_store_size"}}))


    {
        m_util_used_storage.Set(static_cast <double> (m_data_store.get_used_space().get_low()));
        m_util_free_storage.Set(static_cast <double> (m_data_store.get_free_space().get_low()));
        m_config_min_file_size.Set(static_cast <double> (conf.min_file_size));
        m_config_max_file_size.Set(static_cast <double> (conf.max_file_size));
        m_config_max_data_store_size.Set(static_cast <double> (conf.max_data_store_size.get_low()));
    }

    coro <void> handle (messenger m) override {
        for (;;) {
            std::optional<error> err;

            try {
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
                    case STOP:
                        m_is_stopped = true;
                        co_return;
                    default:
                        m_reqs_invalid.Increment();
                        throw std::invalid_argument("Invalid message type!");
                }
            } catch (const error_exception& e) {
                err = e.error();
            } catch (const std::exception& e) {
                err = error(error::unknown, e.what());
            }

            if (err)
            {
                co_await m.send_error (*err);
            }
        }
    }

    bool stop_received() const override {
        return m_is_stopped;
    }

private:

    coro <void> handle_write (messenger &m, const messenger::header& h) {
        unique_buffer <char> data (h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);
        const auto addr = m_data_store.write(data.get_span());
        m_util_used_storage.Set(static_cast <double> (m_data_store.get_used_space().get_low()));
        m_util_free_storage.Set(static_cast <double> (m_data_store.get_free_space().get_low()));
        co_await m.send_address(WRITE_RESP, addr);
    }

    coro <void> handle_read (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        unique_buffer <char> buffer (resp.size);
        const auto size = m_data_store.read(buffer.data(), resp.pointer, resp.size);
        co_await m.send (READ_RESP, {buffer.data(), size});
    }

    coro <void> handle_read_address (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_address(h);

        const auto read_size = std::accumulate (resp.sizes.cbegin(), resp.sizes.cend(), 0ul);

        unique_buffer <char> buffer (read_size);
        size_t offset = 0;
        for (size_t i = 0; i < resp.size(); i++) {
            const auto frag = resp.get_fragment(i);
            if (m_data_store.read(buffer.data() + offset, frag.pointer, frag.size) != frag.size) [[unlikely]] {
                throw std::runtime_error ("Could not read the data with the given size");
            }
            offset += frag.size;
        }
        co_await m.send (READ_ADDRESS_RESP, {buffer.data(), offset});
    }

    coro <void> handle_remove (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        m_data_store.remove(resp.pointer, resp.size);
        m_util_used_storage.Set(static_cast <double> (m_data_store.get_used_space().get_low()));
        m_util_free_storage.Set(static_cast <double> (m_data_store.get_free_space().get_low()));
        co_await m.send (REMOVE_OK, {});
    }

    coro <void> handle_sync (messenger &m, const messenger::header& h) {
        co_await m.recv_address(h);
        m_data_store.sync();
        m_util_used_storage.Set(static_cast <double> (m_data_store.get_used_space().get_low()));
        m_util_free_storage.Set(static_cast <double> (m_data_store.get_free_space().get_low()));
        co_await m.send (SYNC_OK, {});
    }

    coro <void> handle_get_used (messenger &m, const messenger::header&) {
        const auto used = m_data_store.get_used_space();
        co_await m.send_uint128_t(USED_RESP, used);
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

#endif //CORE_DATA_STORE_SERVICE_HANDLER_H
