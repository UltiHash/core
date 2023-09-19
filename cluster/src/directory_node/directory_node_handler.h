//
// Created by masi on 8/29/23.
//

#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/protocol_handler.h"
#include "fdb/fdb.h"

namespace uh::cluster {

class directory_handler: public protocol_handler {
public:

    coro <void> handle (messenger m) override {
        const auto message_header = co_await m.recv_header ();
        switch (message_header.type) {
            case DIR_PUT_OBJ_REQ:
                co_await handle_put_obj (m, message_header);
                break;
            case DIR_GET_OBJ_REQ:
                co_await handle_get_obj (m, message_header);
                break;
            default:
                throw std::invalid_argument ("Invalid message type!");
        }
    }

private:

#if defined(__APPLE__)
    fdb::fdb m_fdb = fdb::fdb("/usr/local/etc/foundationdb/fdb.cluster");
#else
    fdb::fdb m_fdb = fdb::fdb("/etc/foundationdb/fdb.cluster");
#endif
    coro <void> handle_put_obj (messenger& m, const messenger::header& h) {
        std::cout << "received DIR_PUT_OBJ_REQ message" << std::endl;

        directory_request request = co_await m.recv_directory_request(h);
        std::vector<char> addressData;
        zpp::bits::out{addressData}(request.addr).or_throw();

        message_types status = SUCCESS;
        try {
            auto trans = m_fdb.make_transaction();
            trans->put({request.object_key.data(), request.object_key.size()},{addressData.data(), addressData.size()});
            trans->commit();
        } catch (const fdb::fdb_exception e) {
            status = FAILURE;
        }

        co_await m.send(status, {});
        co_return;
    }

    coro <void> handle_get_obj (messenger& m, const messenger::header& h) {
        std::cout << "received DIR_PUT_OBJ_REQ message" << std::endl;
        co_await m.send(DIR_GET_OBJ_REQ, {});
        co_return;
    }
};
} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_HANDLER_H
