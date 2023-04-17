#include <net/tls_server.h>

#include <net/tls_socket.h>
#include <logging/logging_boost.h>


using namespace boost::asio;

namespace uh::net
{

// ---------------------------------------------------------------------

struct tls_server::connection_state
{
    connection_state(io_context& ctx, ssl::context& ssl)
        : sock(ctx, ssl) {}

    ssl::stream<ip::tcp::socket> sock;
    ip::tcp::endpoint peer;
};

// ---------------------------------------------------------------------

tls_server::tls_server(const server_config& config,
                       uh::protocol::protocol_factory& protocol_factory)
    : m_context(),
      m_acceptor(m_context, ip::tcp::endpoint(ip::tcp::v4(), config.port)),
      m_ssl(ssl::context::tlsv13),
      m_protocol_factory(protocol_factory),
      m_scheduler(config.threads)
{
    m_ssl.use_certificate_chain_file(config.tls_chain);
    m_ssl.use_private_key_file(config.tls_pkey, ssl::context::pem);
    m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
}

// ---------------------------------------------------------------------

void tls_server::run()
{
    accept();
    m_context.run();
}

// ---------------------------------------------------------------------

void tls_server::spawn_client(std::shared_ptr<socket> sock)
{
    m_scheduler.spawn([this, sock] ()
    {
        m_protocol_factory.create(sock)->handle();
    });
}

// ---------------------------------------------------------------------

void tls_server::accept()
{
    auto state = std::make_shared<connection_state>(m_context, m_ssl);
    m_acceptor.async_accept(state->sock.lowest_layer(), state->peer,
        [this, state](const std::error_code& ec)
        {
            if (ec)
            {
                ERROR << "accept: " << ec;
            }
            else
            {
                INFO << "new connection: " << state->peer;
                handshake(state, ec);
            }

            accept();
        });
}

// ---------------------------------------------------------------------

void tls_server::handshake(std::shared_ptr<connection_state> state,
                           const std::error_code& ec)
{
    state->sock.async_handshake(ssl::stream_base::server,
        [this, state](const std::error_code& ec)
        {
            if (ec)
            {
                ERROR << "handshake failed for " << state->peer << ": " << ec;
                return;
            }

            auto client = std::make_shared<tls_socket>(std::move(state->sock));
            client->peer() = state->peer;
            spawn_client(client);
        });
}

bool tls_server::is_busy() const {
    return m_scheduler.number_of_threads() == m_scheduler.number_of_busy_threads();
}

// ---------------------------------------------------------------------

} // namespace uh::net
