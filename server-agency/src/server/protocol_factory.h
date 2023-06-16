#ifndef SERVER_AGENCY_SERVER_PROTOCOL_FACTORY_H
#define SERVER_AGENCY_SERVER_PROTOCOL_FACTORY_H

#include <protocol/protocol.h>
#include <metrics/protocol_metrics.h>
#include <metrics/mod.h>
#include <cluster/mod.h>
#include <state/mod.h>
#include <net/server_info.h>


namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol_factory : public uh::protocol::protocol_factory
{
public:
    protocol_factory(
        cluster::mod& cluster,
        metrics::client_metrics_state& client,
        const uh::metrics::protocol_metrics& protocol,
        const uh::net::server_info &serv_info);

    std::unique_ptr<uh::protocol::protocol> create(std::shared_ptr<net::socket> client) override ;

private:
    cluster::mod& m_cluster;
    metrics::client_metrics_state& m_client_metrics;
    const uh::metrics::protocol_metrics& m_protocol_metrics;
    const uh::net::server_info &m_serv_info;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
