#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include <boost/asio.hpp>

#include <memory>


namespace uh::net
{

// ---------------------------------------------------------------------

class connection
{
public:
    connection(boost::asio::io_context& ctx);

    boost::asio::ip::tcp::socket& socket();

    static std::unique_ptr<connection> connect(
        boost::asio::io_context& ctx,
        const std::string& hostname,
        uint16_t port);

private:
    boost::asio::io_context& m_ctx;
    boost::asio::ip::tcp::socket m_socket;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
