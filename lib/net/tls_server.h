#ifndef UH_NET_TLS_SERVER_H
#define UH_NET_TLS_SERVER_H

#include <net/server.h>
#include <net/plain_server.h>

#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>

#include <memory>


namespace uh::net
{

// ---------------------------------------------------------------------

class tls_server : public server
{
public:
    tls_server(const server_config& config,
               uh::protocol::protocol_factory& protocol_factory);

    void run() override;
    [[nodiscard]] bool is_busy () const override;

private:
    struct connection_state;

    void spawn_client(std::shared_ptr<socket> sock);
    void handshake(std::shared_ptr<connection_state> state, const std::error_code& ec);
    void accept();

    boost::asio::io_context m_context;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ssl::context m_ssl;

    uh::protocol::protocol_factory& m_protocol_factory;
    scheduler m_scheduler;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
