#include "client_metrics.h"


namespace uh::an::metrics
{

// ---------------------------------------------------------------------

client_metrics::client_metrics(uh::metrics::service& service)
    : m_gauges(service.add_gauge_family("client_statistics", "Gives statistics about client")),
      m_uhv_id(m_gauges.Add({{"type", "uhv_id"}})),
      m_integrated_size(m_gauges.Add({{"type", "integrated_size"}}))
{
    m_uhv_id.Set(0);
    m_integrated_size.Set(0);
}

// ---------------------------------------------------------------------

prometheus::Gauge& client_metrics::uhv_id() const
{
    return m_uhv_id;
}

// ---------------------------------------------------------------------

prometheus::Gauge& client_metrics::integrated_size() const
{
    return m_integrated_size;
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
