#ifndef SERVER_DATABASE_SERVER_PROTOCOL_FACTORY_H
#define SERVER_DATABASE_SERVER_PROTOCOL_FACTORY_H

#include <net/server.h>
#include <protocol/protocol.h>
#include <metrics/protocol_metrics.h>

#include <storage/mod.h>


namespace uh::dbn::server
{

// ---------------------------------------------------------------------

class protocol_factory : public uh::protocol::protocol_factory
{
public:
    protocol_factory(
        storage::mod& storage,
        const uh::metrics::protocol_metrics& metrics);

    std::unique_ptr<uh::protocol::protocol> create(std::shared_ptr<net::socket> client) override;


private:
    storage::mod& m_storage;
    const uh::metrics::protocol_metrics& m_metrics;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::server

#endif
