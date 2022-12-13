#include "socket.h"


namespace uh::net
{

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

socket_device::socket_device(std::shared_ptr<socket> socket)
    : m_socket(socket)
{
}

// ---------------------------------------------------------------------

std::streamsize socket_device::write(const char* s, std::streamsize n)
{
    return m_socket->write(std::span<const char>(s, n));
}

// ---------------------------------------------------------------------

std::streamsize socket_device::read(char*s, std::streamsize n)
{
    return m_socket->read(std::span<char>(s, n));
}

// ---------------------------------------------------------------------

} // namespace uh::net
