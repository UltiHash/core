#ifndef UH_METRICS_SERVICE_H
#define UH_METRICS_SERVICE_H

#include <memory>
#include <string>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>


namespace uh::metrics
{

// ---------------------------------------------------------------------

struct config
{
    // network address to bind to in the form of `ADDR:PORT`
    std::string address = "0.0.0.0:8080";

    // number of threads
    std::size_t threads = 2;

    // HTTP path to the metrics service
    std::string path = "/metrics";
};

// ---------------------------------------------------------------------

class service
{
public:
    service(const config& cfg);

    prometheus::Family<prometheus::Counter>& add_counter_family(
        const std::string& name,
        const std::string& help = "");

    prometheus::Family<prometheus::Gauge>& add_gauge_family(
        const std::string& name,
        const std::string& help = "");

private:
    prometheus::Exposer m_exposer;
    std::shared_ptr<prometheus::Registry> m_registry;
};

// ---------------------------------------------------------------------

} // namespace uh::metrics

#endif
