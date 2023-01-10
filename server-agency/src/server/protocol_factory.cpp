#include "protocol_factory.h"

#include "protocol.h"


namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(cluster::mod& cluster,
                                   const uh::metrics::protocol_metrics& metrics)
    : m_cluster(cluster),
      m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create()
{
    return std::make_unique<metrics::protocol_metrics_wrapper>(
        m_metrics,
        std::make_unique<protocol>(m_cluster));
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
