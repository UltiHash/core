#include "protocol_factory.h"

#include "protocol.h"


namespace uh::an
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(uh::protocol::client_pool& clients)
    : m_clients(clients)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create() const
{
    return std::make_unique<protocol>(m_clients);
}

// ---------------------------------------------------------------------

} // namespace uh::an
