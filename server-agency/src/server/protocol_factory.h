#ifndef SERVER_AGENCY_SERVER_PROTOCOL_FACTORY_H
#define SERVER_AGENCY_SERVER_PROTOCOL_FACTORY_H

#include <net/server.h>
#include <protocol/protocol.h>
#include <metrics/protocol_metrics.h>

#include <cluster/mod.h>


namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol_factory : public uh::protocol::protocol_factory
{
public:
    protocol_factory(
        cluster::mod& cluster,
        const uh::metrics::protocol_metrics& metrics);

    std::unique_ptr<uh::protocol::protocol> create(std::shared_ptr<net::socket> client) override ;

private:
    cluster::mod& m_cluster;
    const uh::metrics::protocol_metrics& m_metrics;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
