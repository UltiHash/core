#include "protocol_factory.h"

#include "protocol.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(storage_backend& storage, const uh::dbn::metrics& metrics)
    : m_storage(storage),
    m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create()
{
    return std::make_unique<protocol>(m_storage, m_metrics);
}

// ---------------------------------------------------------------------

} // namespace uh::dbn
