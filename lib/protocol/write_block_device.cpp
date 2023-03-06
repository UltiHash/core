#include "write_block_device.h"

#include "exception.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

write_block_device::write_block_device(client& c)
    : m_client(c)
{
}

// ---------------------------------------------------------------------

write_block_device::~write_block_device()
{
    try
    {
        m_client.reset();
    }
    catch (...)
    {
    }
}

// ---------------------------------------------------------------------

std::streamsize write_block_device::write(std::span<const char> buffer)
{
    return m_client.write_chunk(buffer);
}

// ---------------------------------------------------------------------

std::streamsize write_block_device::read(std::span<char> buffer)
{
    THROW(unsupported, "read call not supported on write_block_device");
}

// ---------------------------------------------------------------------

bool write_block_device::valid() const
{
    return m_client.valid();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
