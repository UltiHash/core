//
// Created by masi on 8/29/23.
//

#ifndef CORE_PROTOCOL_HANDLER_H
#define CORE_PROTOCOL_HANDLER_H

#include "network/messenger.h"
#include "metrics_handler.h"
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>


namespace uh::cluster {

    class protocol_handler : protected metrics_handler {
    public:
    virtual coro <void> handle (messenger m) = 0;

    explicit protocol_handler(server_config& c) :
        metrics_handler(c) {};

    ~protocol_handler() override = default;

    [[nodiscard]] virtual bool stop_received() const {
        return false;
    }

};

} // namespace uh::cluster


#endif //CORE_PROTOCOL_HANDLER_H
