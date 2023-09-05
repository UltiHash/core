//
// Created by masi on 8/29/23.
//

#ifndef CORE_DATA_NODE_HANDLER_H
#define CORE_DATA_NODE_HANDLER_H

#include "common/protocol_handler.h"

namespace uh::cluster {

class data_node_handler: public protocol_handler {
public:
    coro <void> handle (messenger m) override {
        co_return;
    }
};

} // end namespace uh::cluster

#endif //CORE_DATA_NODE_HANDLER_H
