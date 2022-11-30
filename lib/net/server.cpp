#include "server.h"

#include <logging/logging_boost.h>


using namespace boost::asio::ip;

namespace uh::net
{

// ---------------------------------------------------------------------

server::server(const server_config& config,
               const util::factory<uh::protocol::protocol>& protocol_factory)
    : m_context(),
      m_acceptor(m_context, tcp::endpoint(tcp::v4(), config.port)),
      m_protocol_factory(protocol_factory),
      m_scheduler(config.threads),
      m_running(false)
{
    m_acceptor.set_option(tcp::acceptor::reuse_address(true));
}

// ---------------------------------------------------------------------

void server::run()
{
    m_running = true;
    while (m_running)
    {
        try
        {
            std::shared_ptr<net::connection> conn = std::make_shared<net::connection>(m_context);
            tcp::endpoint peer;
            m_acceptor.accept(conn->socket(), peer);

            INFO << "new connection: " << peer;

            spawn_client(conn);
        }
        catch (const std::exception& e)
        {
            ERROR << "accept: " << e.what();
        }
    }

    m_scheduler.stop();
}

// ---------------------------------------------------------------------

void server::spawn_client(std::shared_ptr<net::connection> client)
{
    m_scheduler.spawn([this, client] ()
    {
        m_protocol_factory.create()->handle(client);
    });
}

// ---------------------------------------------------------------------

} // namespace uh::net
