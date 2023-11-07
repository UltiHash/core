//
// Created by masi on 8/29/23.
//

#ifndef CORE_PROTOCOL_HANDLER_H
#define CORE_PROTOCOL_HANDLER_H

#include "network/messenger.h"
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>


namespace uh::cluster {

struct protocol_handler {
    virtual coro <void> handle (messenger m) = 0;

    explicit protocol_handler(server_config& c) : m_exposer(make_exposer(c)) {

    }

    virtual ~protocol_handler() = default;



    [[nodiscard]] virtual bool stop_received() const {
        return false;
    }

    private:
    prometheus::Exposer m_exposer;

    prometheus::Exposer make_exposer(const server_config& c)
    {
        LOG_INFO() << "starting metrics server at " << c.metrics_bind_address;
        try
        {
            return prometheus::Exposer(c.metrics_bind_address, c.metrics_threads);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("could not start metrics HTTP server: ") + e.what());
        }
    }
};

} // namespace uh::cluster


#endif //CORE_PROTOCOL_HANDLER_H
