#include "client_factory.h"

#include <net/connection.h>

#include "exception.h"


using namespace boost::asio;

namespace uh::protocol
{

// ---------------------------------------------------------------------

client_factory::client_factory(boost::asio::io_context& context,
                               const client_factory_config& config)
    : m_hostname(config.hostname),
      m_port(config.port),
      m_client_version(config.client_version),
      m_context(context)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<client> client_factory::create() const
{
    auto conn = net::connection::connect(m_context, m_hostname, m_port);

    auto c = std::make_unique<client>(std::move(conn));

    c->hello(m_client_version);

    return std::move(c);
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
