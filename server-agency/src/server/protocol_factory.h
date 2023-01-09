#ifndef SERVER_AGENCY_SERVER_PROTOCOL_FACTORY_H
#define SERVER_AGENCY_SERVER_PROTOCOL_FACTORY_H

#include <net/server.h>
#include <protocol/protocol.h>

#include <cluster/mod.h>
#include <metrics/metrics.h>


namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol_factory : public util::factory<uh::protocol::protocol>
{
public:
    protocol_factory(
        cluster::mod& cluster,
        const an::metrics::metrics& metrics);

    virtual std::unique_ptr<uh::protocol::protocol> create() override;

private:
    cluster::mod& m_cluster;
    const an::metrics::metrics& m_metrics;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
