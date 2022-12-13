#ifndef UH_NET_TLS_SERVER_H
#define UH_NET_TLS_SERVER_H

#include <net/server.h>

#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>

#include <memory>


namespace uh::net
{

// ---------------------------------------------------------------------

class tls_server
{
public:
    tls_server(const server_config& config,
               util::factory<uh::protocol::protocol>& protocol_factory);

    void run();

private:
    struct connection_state;

    void spawn_client(std::shared_ptr<socket> sock);
    void handshake(std::shared_ptr<connection_state> state, const std::error_code& ec);
    void accept();

    boost::asio::io_context m_context;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ssl::context m_ssl;

    util::factory<uh::protocol::protocol>& m_protocol_factory;
    scheduler m_scheduler;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
