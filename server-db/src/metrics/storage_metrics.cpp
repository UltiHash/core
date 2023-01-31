#include "storage_metrics.h"


namespace uh::dbn::metrics
{

// ---------------------------------------------------------------------

storage_metrics::storage_metrics(uh::metrics::service& service)
    : m_gauges(service.add_gauge_family("uh_storage", "storage monitoring")),
    m_free_space(m_gauges.Add({{"type", "free_space"}}))
{
    m_free_space.Set(0);
}

// ---------------------------------------------------------------------

prometheus::Gauge& storage_metrics::free_space() const{
    return m_free_space;
}

} // namespace uh::metrics
