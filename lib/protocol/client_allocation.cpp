#include "client_allocation.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

client_allocation::client_allocation(client& c)
    : m_client(c),
      m_device(c)
{
}

// ---------------------------------------------------------------------

io::device& client_allocation::device()
{
    return m_device;
}

// ---------------------------------------------------------------------

block_meta_data client_allocation::persist()
{
    return m_client.finalize();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
