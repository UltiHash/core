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

std::unique_ptr<uh::protocol::protocol> protocol_factory::create()
{
    return std::make_unique<metrics::protocol_metrics_wrapper>(
        m_metrics,
        std::make_unique<protocol>(m_storage));
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::server
