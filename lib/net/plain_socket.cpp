#include <net/plain_socket.h>

#include <util/exception.h>


using namespace boost::asio;

namespace uh::net
{

// ---------------------------------------------------------------------

plain_socket::plain_socket(boost::asio::ip::tcp::socket&& sock)
    : m_sock(std::move(sock))
{
}

// ---------------------------------------------------------------------

std::unique_ptr<plain_socket> plain_socket::connect(io_context& ctx,
                                                    const std::string& hostname,
                                                    uint16_t port)
{
    ip::tcp::resolver resolver(ctx);
    auto result = resolver.resolve(hostname, "");

    if (result.empty())
    {
        THROW(util::exception, "failure during hostname lookup: " + hostname);
    }

    auto endpoint = result.begin()->endpoint();
    endpoint.port(port);

    ip::tcp::socket sock(ctx);
    sock.connect(endpoint);

    std::unique_ptr<plain_socket> conn = std::make_unique<plain_socket>(std::move(sock));
    conn->peer() = endpoint;

    return std::move(conn);
}

// ---------------------------------------------------------------------

std::streamsize plain_socket::write_impl(std::span<const char> buffer)
{
    return m_sock.write_some(const_buffer{ buffer.data(), buffer.size() });
}

// ---------------------------------------------------------------------

std::streamsize plain_socket::read_impl(std::span<char> buffer)
{
    return m_sock.read_some(mutable_buffer{ buffer.data(), buffer.size() });
}

// ---------------------------------------------------------------------

plain_socket_factory::plain_socket_factory(
    boost::asio::io_context& ctx,
    const std::string& hostname,
    uint16_t port)
    : m_ctx(ctx),
      m_hostname(hostname),
      m_port(port)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<socket> plain_socket_factory::create()
{
    return plain_socket::connect(m_ctx, m_hostname, m_port);
}

// ---------------------------------------------------------------------

} // namespace uh::net
