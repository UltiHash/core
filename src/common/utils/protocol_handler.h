//
// Created by masi on 8/29/23.
//

#ifndef CORE_PROTOCOL_HANDLER_H
#define CORE_PROTOCOL_HANDLER_H

#include "common/network/messenger.h"

namespace uh::cluster {

    class protocol_handler  {
    public:
    virtual coro <void> handle (messenger m) = 0;

    virtual ~protocol_handler() = default;

    [[nodiscard]] virtual bool stop_received() const {
        return false;
    }

};

} // namespace uh::cluster


#endif //CORE_PROTOCOL_HANDLER_H
