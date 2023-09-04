//
// Created by masi on 8/29/23.
//

#ifndef CORE_PHONEBOOK_NODE_HANDLER_H
#define CORE_PHONEBOOK_NODE_HANDLER_H

#include "common/protocol_handler.h"

namespace uh::cluster {

class phonebook_node_handler: public protocol_handler {
public:

    coro <void> handle (messenger m) override {
        co_return;
    }
};

} // end namespace uh::cluster

#endif //CORE_PHONEBOOK_NODE_HANDLER_H
