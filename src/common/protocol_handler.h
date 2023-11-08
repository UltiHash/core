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

class protocol_handler {
    public:
    virtual coro <void> handle (messenger m) = 0;

    explicit protocol_handler(server_config& c) :
        m_exposer(make_exposer(c)),
        m_registry(std::make_shared<prometheus::Registry>()),
        m_metrics_path(c.metrics_path) {};

    virtual ~protocol_handler() = default;

    [[nodiscard]] virtual bool stop_received() const {
        return false;
    }

    protected:
    prometheus::Family<prometheus::Counter>& add_counter_family(const std::string& name,
                                                                const std::string& help)
    {
        auto builder = prometheus::BuildCounter().Name(name).Help(help);

        return builder.Register(*m_registry);
    }

    prometheus::Family<prometheus::Gauge>& add_gauge_family(const std::string& name,
                                                            const std::string& help)
    {
        auto builder = prometheus::BuildGauge().Name(name).Help(help);

        return builder.Register(*m_registry);
    }

    void init() {
        m_exposer.RegisterCollectable(m_registry, m_metrics_path);
    }

    private:
    prometheus::Exposer m_exposer;
    std::shared_ptr<prometheus::Registry> m_registry;
    std::string m_metrics_path;

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
