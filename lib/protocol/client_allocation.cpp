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

blob client_allocation::persist()
{
    return m_client.finalize();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
