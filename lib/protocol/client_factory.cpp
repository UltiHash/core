#include "client_factory.h"

#include <net/plain_socket.h>

#include "exception.h"


using namespace boost::asio;

namespace uh::protocol
{

// ---------------------------------------------------------------------

client_factory::client_factory(
    std::unique_ptr<util::factory<net::socket>>&& socket_factory,
    const client_factory_config& config)
    : m_sf(std::move(socket_factory)),
      m_client_version(config.client_version)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<client> client_factory::create()
{
    auto conn = m_sf->create();

    auto c = std::make_unique<client>(std::move(conn));

    c->hello(m_client_version);

    return c;
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
