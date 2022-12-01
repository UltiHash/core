#include "protocol_factory.h"

#include "protocol.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

protocol_factory::protocol_factory(storage_backend_interface& storage)
    : m_storage(storage)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create() const
{
    return std::make_unique<protocol>(m_storage);
}

// ---------------------------------------------------------------------

} // namespace uh::dbn
