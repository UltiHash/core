#ifndef NET_CONNECTION_H
#define NET_CONNECTION_H

#include <boost/asio.hpp>


namespace uh::net
{

// ---------------------------------------------------------------------

class connection
{
public:
    connection(boost::asio::io_context& ctx)
        : m_ctx(ctx),
          m_socket(ctx)
    {
    }

    boost::asio::ip::tcp::socket& socket() { return m_socket; }
private:
    boost::asio::io_context& m_ctx;
    boost::asio::ip::tcp::socket m_socket;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
