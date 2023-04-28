#include "protocol_factory.h"
#include "protocol/server.h"
#include "protocol.h"


namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(cluster::mod& cluster,
                                   metrics::mod& client,
                                   const uh::metrics::protocol_metrics& metrics,
                                   const uh::net::server_info &serv_info)
    : m_cluster(cluster),
      m_client(client),
      m_metrics(metrics),
      m_serv_info (serv_info)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create(std::shared_ptr<net::socket> client)
{
    return std::make_unique<uh::protocol::server>
            (client,
                    std::make_unique<uh::metrics::protocol_metrics_wrapper>
                            (m_metrics, std::make_unique<uh::an::server::protocol>(m_cluster, m_client.client(), m_serv_info)));
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
