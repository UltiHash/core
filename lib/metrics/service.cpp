#include "service.h"

#include <util/exception.h>


using namespace prometheus;

namespace uh::metrics
{

namespace
{

// ---------------------------------------------------------------------

prometheus::Exposer make_exposer(const config& c)
{
    try
    {
        return prometheus::Exposer(c.address, c.threads);
    }
    catch (const std::exception& e)
    {
        THROW(util::exception,
              std::string("could not start metrics HTTP server: ") + e.what());
    }
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

service::service(const config& cfg)
    : m_exposer(make_exposer(cfg)),
      m_registry(std::make_shared<prometheus::Registry>())
{
    m_exposer.RegisterCollectable(m_registry, cfg.path);
}

// ---------------------------------------------------------------------

prometheus::Family<Counter>& service::add_counter_family(const std::string& name,
                                                         const std::string& help)
{
    auto builder = BuildCounter().Name(name).Help(help);

    return builder.Register(*m_registry);
}

// ---------------------------------------------------------------------

prometheus::Family<Gauge>& service::add_gauge_family(const std::string& name,
                                                     const std::string& help)
{
    auto builder = BuildGauge().Name(name).Help(help);

    return builder.Register(*m_registry);
}

// ---------------------------------------------------------------------

} // namespace uh::metrics
