#include "protocol_factory.h"
#include "protocol/server.h"
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

std::unique_ptr<uh::protocol::protocol> protocol_factory::create(std::shared_ptr<net::socket> client)
{
    return std::make_unique<uh::protocol::server>
            (client,
                    std::make_unique<uh::metrics::protocol_metrics_wrapper>
                            (m_metrics, std::make_unique<uh::an::server::protocol>(m_cluster)));
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
