//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_INTERNAL_HANDLER_H
#define CORE_ENTRY_NODE_INTERNAL_HANDLER_H

#include "common/protocol_handler.h"

namespace uh::cluster {

class entry_node_internal_handler: public protocol_handler {
public:

    entry_node_internal_handler(entry_node_config conf, int id) :
    protocol_handler(conf.internal_server_conf) {
        init();
    }

    coro <void> handle (messenger m) override {
        co_return;
    }
};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_INTERNAL_HANDLER_H
