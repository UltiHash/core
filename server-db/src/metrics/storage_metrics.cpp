#include "storage_metrics.h"


namespace uh::dbn::metrics
{

// ---------------------------------------------------------------------

storage_metrics::storage_metrics(uh::metrics::service& service)
    : m_gauges(service.add_gauge_family("uh_storage", "storage monitoring")),
      m_free_space(m_gauges.Add({{"type", "free_space"}})),
      m_used_space(m_gauges.Add({{"type", "used_space"}})),
      m_alloc_space(m_gauges.Add({{"type", "allocated_space"}})),

      m_gauge_comp(service.add_gauge_family("uh_compression", "compression monitoring")),
      m_comp_scheduled(m_gauge_comp.Add({{"type", "scheduled"}})),
      m_comp_running(m_gauge_comp.Add({{"type", "running"}}))
{
    m_free_space.Set(0);
    m_used_space.Set(0);
    m_alloc_space.Set(0);
    m_comp_scheduled.Set(0);
    m_comp_running.Set(0);
}

// ---------------------------------------------------------------------

prometheus::Gauge& storage_metrics::free_space() const
{
    return m_free_space;
}

// ---------------------------------------------------------------------

prometheus::Gauge& storage_metrics::used_space() const
{
    return m_used_space;
}

// ---------------------------------------------------------------------

prometheus::Gauge& storage_metrics::alloc_space() const
{
    return m_alloc_space;
}

// ---------------------------------------------------------------------

prometheus::Gauge& storage_metrics::comp_scheduled() const
{
    return m_comp_scheduled;
}

// ---------------------------------------------------------------------

prometheus::Gauge& storage_metrics::comp_running() const
{
    return m_comp_running;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::metrics
