#include "protocol_factory.h"
#include "protocol/server.h"
#include "protocol.h"


namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(cluster::mod& cluster,
                                   metrics::client_metrics_state& client,
                                   const uh::metrics::protocol_metrics& protocol,
                                   const uh::net::server_info &serv_info)
    : m_cluster(cluster),
      m_client_metrics(client),
      m_protocol_metrics(protocol),
      m_serv_info (serv_info)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create(std::shared_ptr<net::socket> client)
{
    return std::make_unique<uh::protocol::server>
            (client,
                    std::make_unique<uh::metrics::protocol_metrics_wrapper>
                            (m_protocol_metrics, std::make_unique<uh::an::server::protocol>
                                    (m_cluster, m_client_metrics, m_serv_info)));

}

// ---------------------------------------------------------------------

} // namespace uh::an::server
