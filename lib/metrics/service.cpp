#include "service.h"


using namespace prometheus;

namespace uh::metrics
{

// ---------------------------------------------------------------------

service::service(const config& cfg)
    : m_exposer(cfg.address, cfg.threads),
      m_registry(std::make_shared<prometheus::Registry>())
{
    m_exposer.RegisterCollectable(m_registry, cfg.path);
}

// ---------------------------------------------------------------------

prometheus::Family<Counter>& service::add_counter_family(const std::string& name,
                                                         const std::string& help)
{
    auto& family = BuildCounter().Name(name);

    if (!help.empty())
    {
        family.Help(help);
    }

    return family.Register(*m_registry);
}

// ---------------------------------------------------------------------

prometheus::Family<Gauge>& service::add_gauge_family(const std::string& name,
                                                     const std::string& help)
{
    auto& family = BuildGauge().Name(name);

    if (!help.empty())
    {
        family.Help(help);
    }

    return family.Register(*m_registry);
}

// ---------------------------------------------------------------------

} // namespace uh::metrics
