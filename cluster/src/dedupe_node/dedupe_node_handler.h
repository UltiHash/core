//
// Created by masi on 8/29/23.
//

#ifndef CORE_DEDUPE_NODE_HANDLER_H
#define CORE_DEDUPE_NODE_HANDLER_H

#include "common/protocol_handler.h"

namespace uh::cluster {

class dedupe_node_handler: public protocol_handler {
    void handle (messenger m) override {

    }
};

} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_HANDLER_H
