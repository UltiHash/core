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

    data_node_handler (data_node_config conf, int id):
    m_data_store (std::move(conf), id)
    {}

    coro <void> handle (messenger m) override {
        for (;;) {
            const auto message_header = co_await m.recv_header();
            switch (message_header.type) {
                case WRITE_REQ:
                    co_await handle_write(m, message_header);
                    break;
                case READ_REQ:
                    co_await handle_read(m, message_header);
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
                    co_return;
                default:
                    throw std::invalid_argument("Invalid message type!");
            }
        }
    }

private:

    coro <void> handle_write (messenger &m, const messenger::header& h) {
        ospan <char> data (h.size);
        m.register_read_buffer(data);
        co_await m.recv_buffers(h);
        const auto addr = m_data_store.write({data.data.get(), data.size});
        co_await m.send_address(WRITE_RESP, addr);
    }

    coro <void> handle_read (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        ospan <char> buffer (resp.second.size);
        const auto size = m_data_store.read(buffer.data.get(), resp.second.pointer, resp.second.size);
        co_await m.send (READ_RESP, {buffer.data.get(), size});
    }

    coro <void> handle_remove (messenger &m, const messenger::header& h) {
        const auto resp = co_await m.recv_fragment(h);
        m_data_store.remove(resp.second.pointer, resp.second.size);
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

};

} // end namespace uh::cluster

#endif //CORE_DATA_NODE_HANDLER_H
