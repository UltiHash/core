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
    }
};

} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_HANDLER_H
