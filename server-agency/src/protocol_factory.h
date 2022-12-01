#ifndef SERVER_AGENCY_PROTOCOL_FACTORY_H
#define SERVER_AGENCY_PROTOCOL_FACTORY_H

#include <net/server.h>
#include <protocol/client_pool.h>


namespace uh::an
{

// ---------------------------------------------------------------------

class protocol_factory : public util::factory<uh::protocol::protocol>
{
public:
    protocol_factory(uh::protocol::client_pool& clients);

    virtual std::unique_ptr<uh::protocol::protocol> create() const override;

private:
    uh::protocol::client_pool& m_clients;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
