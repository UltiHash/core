#include "agency_metrics.h"


namespace uh::an::metrics
{

// ---------------------------------------------------------------------

agency_metrics::agency_metrics(uh::metrics::service& service)
    : m_gauges(service.add_gauge_family("client_stat", "Client Statistics")),
      m_uhv_id(m_gauges.Add({{"type", "uhv_id"}})),
      m_integrated_size(m_gauges.Add({{"type", "integrated_size"}}))
{
    m_uhv_id.Set(0);
    m_integrated_size.Set(0);
}

// ---------------------------------------------------------------------

prometheus::Gauge& agency_metrics::uhv_id() const
{
    return m_uhv_id;
}

// ---------------------------------------------------------------------

prometheus::Gauge& agency_metrics::integrated_size() const
{
    return m_integrated_size;
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
