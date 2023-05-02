#include "client_metrics.h"


namespace uh::an::metrics
{

// ---------------------------------------------------------------------

client_metrics::client_metrics(uh::metrics::service& service)
    : m_gauges(service.add_gauge_family("client_metrics", "Gives the integrated size of the associated UHV file"))
{
}

// ---------------------------------------------------------------------

void client_metrics::set_uhv_metrics(const std::pair<std::string, std::uint64_t>& label) const
{
    m_gauges.Add({{"uhv_id", label.first}}).Set(static_cast<double>(label.second));
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
