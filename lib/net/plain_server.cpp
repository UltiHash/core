#include "plain_server.h"

#include <logging/logging_boost.h>
#include <net/plain_socket.h>


using namespace boost::asio::ip;

namespace uh::net
{

// ---------------------------------------------------------------------

plain_server::plain_server(
    const server_config& config,
    uh::protocol::protocol_factory& protocol_factory)
    : m_context(),
      m_acceptor(m_context, tcp::endpoint(tcp::v4(), config.port)),
      m_protocol_factory(protocol_factory),
      m_scheduler(config.threads),
      m_running(false)
{
    m_acceptor.set_option(tcp::acceptor::reuse_address(true));
}

// ---------------------------------------------------------------------

void plain_server::run()
{
    m_running = true;
    while (m_running)
    {
        try
        {
            tcp::socket s(m_context);
            tcp::endpoint peer;
            m_acceptor.accept(s, peer);

            INFO << "new connection: " << peer;

            auto sock = std::make_shared<plain_socket>(std::move(s));
            sock->peer() = peer;
            spawn_client(sock);
        }
        catch (const std::exception& e)
        {
            ERROR << "accept: " << e.what();
        }
    }

    m_scheduler.stop();
}

// ---------------------------------------------------------------------

void plain_server::spawn_client(const std::shared_ptr<net::socket>& sock)
{
    m_scheduler.spawn([this, sock] ()
    {

        m_protocol_factory.create(sock)->handle();
    });
}

// ---------------------------------------------------------------------

bool plain_server::is_busy() const
{
    return m_scheduler.number_of_threads() == m_scheduler.number_of_busy_threads();
}

// ---------------------------------------------------------------------

} // namespace uh::net
