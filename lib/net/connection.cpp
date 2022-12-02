#include "connection.h"

#include <util/exception.h>


namespace uh::net
{

// ---------------------------------------------------------------------

connection::connection(boost::asio::io_context& ctx)
    : m_ctx(ctx),
      m_socket(ctx)
{
}

// ---------------------------------------------------------------------

boost::asio::ip::tcp::socket& connection::socket()
{
    return m_socket;
}

// ---------------------------------------------------------------------

std::unique_ptr<connection> connection::connect(
    boost::asio::io_context& ctx,
    const std::string& hostname,
    uint16_t port)
{
    boost::asio::ip::tcp::resolver resolver(ctx);
    auto result = resolver.resolve(hostname, "");

    if (result.empty())
    {
        THROW(util::exception, "failure during hostname lookup: " + hostname);
    }

    std::unique_ptr<connection> conn = std::make_unique<connection>(ctx);

    auto endpoint = result.begin()->endpoint();
    endpoint.port(port);
    conn->socket().connect(endpoint);

    return std::move(conn);
}

// ---------------------------------------------------------------------

} // namespace uh::net
