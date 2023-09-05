//
// Created by masi on 8/29/23.
//

#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/protocol_handler.h"

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
    coro <void> handle_put_obj (messenger& m, const messenger::header& h) {
        std::cout << "received DIR_PUT_OBJ_REQ message" << std::endl;
        co_await m.send(SUCCESS, {});
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
