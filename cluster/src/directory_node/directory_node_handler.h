//
// Created by masi on 8/29/23.
//

#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/protocol_handler.h"
#include "directory_store.h"

namespace uh::cluster {

class directory_handler: public protocol_handler {
public:

    directory_handler (directory_store_config conf, global_data_view& storage):
    m_directory (std::move (conf)),
    m_storage (storage) {}

    coro <void> handle (messenger m) override {
        for (;;) {
            const auto message_header = co_await m.recv_header ();
            switch (message_header.type) {
                case DIR_PUT_OBJ_REQ:
                    co_await handle_put_obj (m, message_header);
                    break;
                case DIR_GET_OBJ_REQ:
                    co_await handle_get_obj (m, message_header);
                    break;
                case DIR_PUT_BUCKET_REQ:
                    co_await handle_put_bucket (m, message_header);
                    break;
                case STOP:
                    co_return;
                default:
                    throw std::invalid_argument ("Invalid message type!");
            }
        }

    }

private:

    coro <void> handle_put_obj (messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message (h);

        std::string failure;
        std::vector<char> address_data;
        zpp::bits::out{address_data, zpp::bits::size4b{}}(*request.addr).or_throw();
        try {
            m_directory.insert (request.bucket_id, *request.object_key, address_data);
            co_await m.send(SUCCESS, {});
        } catch (const std::exception& e) {
            failure = std::string (e.what());
        }

        if(!failure.empty())
            co_await m.send(FAILURE, failure);

        co_return;
    }

    coro <void> handle_get_obj (messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message (h);
        address addr;

        std::string failure;
        try {
            const auto buf = m_directory.get(request.bucket_id, *request.object_key);
            zpp::bits::in{std::span <char> {buf.data.get(), buf.size}, zpp::bits::size4b{}}(addr).or_throw();
        } catch (const std::exception& e) {
            failure = std::string (e.what());
        }

        if(!failure.empty())
            co_await m.send(FAILURE, failure);

        std::size_t buffer_size = 0;
        for(auto frag_size : addr.sizes){
            buffer_size += frag_size;
        }

        ospan <char> buffer (buffer_size);

        std::size_t buffer_offset = 0;
        for(int i = 0; i < addr.size(); i++) {
            auto frag = addr.get_fragment(i);
            co_await m_storage.read(buffer.data.get() + buffer_offset, frag.pointer, frag.size);
            buffer_offset += frag.size;
        }

        m.register_write_buffer(buffer);
        co_await m.send_buffers(DIR_GET_OBJ_RESP);

        co_return;
    }

    coro <void> handle_put_bucket (messenger& m, const messenger::header& h) {
        directory_message request = co_await m.recv_directory_message (h);
        std::string failure;

        try {
            m_directory.add_bucket(request.bucket_id);
            co_await m.send(SUCCESS, {});
        } catch (const std::exception& e) {
            failure = std::string (e.what());
        }

        if(!failure.empty())
            co_await m.send(FAILURE, failure);
    }

    directory_store m_directory;
    global_data_view& m_storage;
};
} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_HANDLER_H
