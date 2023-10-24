//
// Created by masi on 8/29/23.
//

#ifndef CORE_PROTOCOL_HANDLER_H
#define CORE_PROTOCOL_HANDLER_H

#include "network/messenger.h"

namespace uh::cluster {

struct protocol_handler {
    virtual coro <void> handle (messenger m) = 0;

    virtual ~protocol_handler() = default;

    virtual bool stop_received() {
        return false;
    }
};

} // namespace uh::cluster


#endif //CORE_PROTOCOL_HANDLER_H
