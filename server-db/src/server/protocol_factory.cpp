#include "protocol_factory.h"

#include "protocol.h"


namespace uh::dbn::server
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(storage::mod& storage,
                                   const uh::metrics::protocol_metrics& metrics)
    : m_storage(storage),
      m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create(std::shared_ptr<net::socket> client)
{
    return std::make_unique<uh::metrics::protocol_metrics_wrapper>(
            client,
            m_metrics,
            std::make_unique<protocol>(m_storage.backend(), client));
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::server
