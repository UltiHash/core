#include <net/tls_socket.h>

#include <util/exception.h>


using namespace boost::asio;

namespace uh::net
{

// ---------------------------------------------------------------------

tls_socket::tls_socket(ssl::stream<ip::tcp::socket>&& sock)
    : m_sock(std::move(sock))
{
}

// ---------------------------------------------------------------------

std::unique_ptr<tls_socket> tls_socket::connect(io_context& ctx,
                                                ssl::context& ssl,
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

    ssl::stream<ip::tcp::socket> sock(ctx, ssl);

    sock.lowest_layer().connect(endpoint);
    sock.handshake(ssl::stream<ip::tcp::socket>::client);

    return std::make_unique<tls_socket>(std::move(sock));
}

// ---------------------------------------------------------------------

std::streamsize tls_socket::write_impl(std::span<const char> buffer)
{
    return m_sock.write_some(const_buffer{ buffer.data(), buffer.size() });
}

// ---------------------------------------------------------------------

std::streamsize tls_socket::read_impl(std::span<char> buffer)
{
    return m_sock.read_some(mutable_buffer{ buffer.data(), buffer.size() });
}

// ---------------------------------------------------------------------

tls_socket_factory::tls_socket_factory(io_context& ctx,
                                       ssl::context& ssl,
                                       const std::string& hostname,
                                       uint16_t port)
    : m_ctx(ctx),
      m_ssl(ssl),
      m_hostname(hostname),
      m_port(port)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<socket> tls_socket_factory::create()
{
    return tls_socket::connect(m_ctx, m_ssl, m_hostname, m_port);
}

// ---------------------------------------------------------------------

} // namespace uh::net
