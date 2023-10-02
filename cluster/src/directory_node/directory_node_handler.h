//
// Created by masi on 8/29/23.
//

#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/protocol_handler.h"
#include "directory_store.h"
#include "fdb/fdb.h"

namespace uh::cluster {

class directory_handler: public protocol_handler {
public:

    explicit directory_handler (global_data& storage):
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
                case STOP:
                    co_return;
                default:
                    throw std::invalid_argument ("Invalid message type!");
            }
        }

    }

private:

#if defined(__APPLE__)
    fdb::fdb m_fdb = fdb::fdb("/usr/local/etc/foundationdb/fdb.cluster");
#else
    fdb::fdb m_fdb = fdb::fdb("/etc/foundationdb/fdb.cluster");
#endif
    coro <void> handle_put_obj (messenger& m, const messenger::header& h) {
        directory_put_request request = co_await m.recv_directory_put_object_request(h);
        std::vector<char> addressData;
        zpp::bits::out{addressData}(request.addr).or_throw();

        bool failure = false;
        try {
            auto trans = m_fdb.make_transaction();
            trans->put({request.object_key.data(), request.object_key.size()},{addressData.data(), addressData.size()});
            trans->commit();
            co_await m.send(SUCCESS, {});
        } catch (const fdb::fdb_exception& e) {
            failure = true;
        }

        if(failure)
            co_await m.send(FAILURE, {});


        co_return;
    }

    coro <void> handle_get_obj (messenger& m, const messenger::header& h) {
        directory_get_request request = co_await m.recv_directory_get_object_request(h);
        address addr;

        bool failure = false;
        try {
            auto trans = m_fdb.make_transaction();
            auto result = trans->get({request.object_key.data(), request.object_key.size()});

            if (!result.has_value()) {
                co_await m.send(FAILURE, {});
            }

            zpp::bits::in{ result->value}(addr).or_throw();
        } catch (const fdb::fdb_exception& e) {
            failure = true;
        }

        if(failure)
            co_await m.send(FAILURE, {});

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

    global_data& m_storage;
};
} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_HANDLER_H
