#include "client_metrics.h"

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

client_metrics::client_metrics(uh::metrics::service& service,
                               const uh::an::persistence::client_metrics& persisted_client_metrics)
    : m_gauges(service.add_gauge_family("client_metrics", "Gives the integrated size of the associated UHV file"))
{
    for (const auto& [id, size] : persisted_client_metrics.id_to_size_map())
    {
        m_gauges.Add({{"uhv_id", id}})
                .Set(static_cast<double>(size));
    }
}

// ---------------------------------------------------------------------

void client_metrics::set_uhv_metrics(const uh::protocol::client_statistics::request& client_stat) const
{
    m_gauges.Add({{"uhv_id", std::string(client_stat.uhv_id.begin(), client_stat.uhv_id.end())}})
        .Set(static_cast<double>(client_stat.integrated_size));
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
