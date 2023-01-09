#include "protocol_factory.h"

#include "protocol.h"


namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(uh::protocol::client_pool& clients,
                                   const uh::an::metrics::metrics& metrics)
    : m_clients(clients),
      m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create()
{
    return std::make_unique<protocol>(m_clients, m_metrics);
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
