#include "client_allocation.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

client_allocation::client_allocation(client& c)
    : m_client(c),
      m_device(c),
      m_dangling(true)
{
}

// ---------------------------------------------------------------------

client_allocation::~client_allocation()
{
    try
    {
        if (m_dangling)
        {
            m_client.reset();
        }
    }
    catch (const std::exception&)
    {
    }
}

// ---------------------------------------------------------------------

io::device& client_allocation::device()
{
    return m_device;
}

// ---------------------------------------------------------------------

block_meta_data client_allocation::persist()
{
    m_dangling = false;
    return m_client.finalize();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
