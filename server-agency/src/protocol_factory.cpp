#include "protocol_factory.h"

#include "protocol.h"


namespace uh::an
{

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::protocol> protocol_factory::create() const
{
    return std::make_unique<protocol>();
}

// ---------------------------------------------------------------------

} // namespace uh::an
