//
// Created by masi on 8/29/23.
//

#ifndef CORE_DIRECTORY_NODE_HANDLER_H
#define CORE_DIRECTORY_NODE_HANDLER_H

#include "common/protocol_handler.h"

namespace uh::cluster {

class directory_handler: public protocol_handler {
public:

    void handle (messenger m) override {
        auto message = m.recv();
        switch (message.first) {
            case DIR_PUT_OBJ_REQ:
                handle_put_obj (m, std::move (message.second));
                break;
            default:
                throw std::invalid_argument ("Invalid message type!");
        }
    }

private:
    void handle_put_obj (messenger& m, ospan<char> data) {
        const auto result = deduplicate ({data.data.get(), data.size});
        m.send (DIR_PUT_OBJ_RESP, {reinterpret_cast <const char*> (result.second.data()), result.second.size()});
    }
};

} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_HANDLER_H
