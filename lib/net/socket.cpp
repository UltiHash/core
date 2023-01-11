#include "socket.h"


namespace uh::net
{

// ---------------------------------------------------------------------

std::streamsize socket::write(std::span<const char> buffer)
{
    try
    {
        return write_impl(buffer);
    }
    catch (const boost::system::system_error& e)
    {
        m_valid = false;
        throw;
    }
}

// ---------------------------------------------------------------------

std::streamsize socket::read(std::span<char> buffer)
{
    try
    {
        return read_impl(buffer);
    }
    catch (const boost::system::system_error& e)
    {
        m_valid = false;
        throw;
    }
}

// ---------------------------------------------------------------------

const boost::asio::ip::tcp::endpoint& socket::peer() const
{
    return m_peer;
}

// ---------------------------------------------------------------------

boost::asio::ip::tcp::endpoint& socket::peer()
{
    return m_peer;
}

// ---------------------------------------------------------------------

bool socket::valid() const
{
    return m_valid;
}

// ---------------------------------------------------------------------

} // namespace uh::net
