#include "client_metrics.h"

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

client_metrics::client_metrics(uh::metrics::service& service)
    : m_gauges(service.add_gauge_family("uh_client", "Gives the integrated size of the associated UHV file"))
{
}

// ---------------------------------------------------------------------

void client_metrics::set_uhv_metrics(const uh::protocol::client_statistics::request& client_stat) const
{
    m_gauges.Add({{"uhv_id", std::string(client_stat.uhv_id.begin(), client_stat.uhv_id.end())}})
        .Set(static_cast<double>(client_stat.integrated_size));
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
