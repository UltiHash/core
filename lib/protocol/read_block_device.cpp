#include "read_block_device.h"

#include <logging/logging_boost.h>

#include "client.h"
#include "exception.h"



namespace uh::protocol
{

// ---------------------------------------------------------------------

read_block_device::read_block_device(client& c)
    : m_client(c)
{
}

// ---------------------------------------------------------------------

read_block_device::~read_block_device()
{
    try
    {
        m_client.reset();
    }
    catch (const std::exception& e)
    {
        WARNING << "error while read client reset: " << e.what();
    }
    catch (...)
    {
        WARNING << "unknown error while read client reset";
    }
}

// ---------------------------------------------------------------------

std::streamsize read_block_device::write(std::span<const char> buffer)
{
    THROW(unsupported, "write call not supported on read_block_device");
}

// ---------------------------------------------------------------------

std::streamsize read_block_device::read(std::span<char> buffer)
{
    return m_client.next_chunk(buffer);
}

// ---------------------------------------------------------------------

bool read_block_device::valid() const
{
    return m_client.valid();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
