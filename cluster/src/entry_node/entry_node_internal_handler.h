//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_INTERNAL_HANDLER_H
#define CORE_ENTRY_NODE_INTERNAL_HANDLER_H

#include "common/protocol_handler.h"

namespace uh::cluster {

class entry_node_internal_handler: public protocol_handler {
public:

    void handle (messenger m) override {

    }
};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_INTERNAL_HANDLER_H
