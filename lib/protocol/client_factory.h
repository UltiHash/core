#ifndef PROTOCOL_CLIENT_FACTORY_H
#define PROTOCOL_CLIENT_FACTORY_H

#include <util/factory.h>
#include "client.h"

#include <boost/asio.hpp>


namespace uh::protocol
{

// ---------------------------------------------------------------------

struct client_factory_config
{
    std::string client_version;
};

// ---------------------------------------------------------------------

class client_factory : public util::factory<client>
{
public:
    client_factory(std::unique_ptr<util::factory<net::socket>>&& socket_factory,
                   const client_factory_config& config);

    virtual std::unique_ptr<client> create() override;

private:
    std::unique_ptr<util::factory<net::socket>> m_sf;
    std::string m_client_version;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
