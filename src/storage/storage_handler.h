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

    storage_handler(storage_config config, size_t index) :
            m_data_store(std::move(config), index),
            m_is_stopped(false)
    {
    }

    coro <void> handle (messenger m) override {
        for (;;) {
            std::optional<error> err;

            try {
                const auto message_header = co_await m.recv_header();
                switch (message_header.type) {
                    case WRITE_REQ:
                        co_await handle_write(m, message_header);
                        break;
                    case READ_REQ:
                        co_await handle_read(m, message_header);
                        break;
                    case READ_ADDRESS_REQ:
                        co_await handle_read_address (m, message_header);
                        break;
                    case REMOVE_REQ:
                        co_await handle_remove(m, message_header);
                        break;
                    case SYNC_REQ:
                        co_await handle_sync(m, message_header);
                        break;
                    case USED_REQ:
                        co_await handle_get_used(m, message_header);
                        break;
                    case STOP:
                        m_is_stopped = true;
                        co_return;
                    default:
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
        co_await m.send (REMOVE_OK, {});
    }

    coro <void> handle_sync (messenger &m, const messenger::header& h) {
        co_await m.recv_address(h);
        m_data_store.sync();
        co_await m.send (SYNC_OK, {});
    }

    coro <void> handle_get_used (messenger &m, const messenger::header& h) {
        const auto used = m_data_store.get_used_space();
        co_await m.send_uint128_t(USED_RESP, used);
    }

    uh::cluster::data_store m_data_store;
    bool m_is_stopped;
};

} // end namespace uh::cluster

#endif //CORE_DATA_STORE_SERVICE_HANDLER_H
