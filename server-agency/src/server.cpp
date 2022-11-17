#include "server.h"

#include <logging/logging_boost.h>

#include <iostream>
#include <boost/bind/bind.hpp>


using namespace boost::asio::ip;

namespace uh::an
{

// ---------------------------------------------------------------------

server::server(const server_config& config,
               const util::factory<protocol>& protocol_factory)
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
            std::shared_ptr<connection> conn = std::make_shared<connection>(m_context);
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

void server::spawn_client(std::shared_ptr<connection> client)
{
    m_scheduler.spawn([this, client] ()
    {
        m_protocol_factory.create()->handle(client);
    });
}

// ---------------------------------------------------------------------

} // namespace uh::an
