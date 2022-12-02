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
    std::string hostname;
    uint16_t port;
    std::string client_version;
};

// ---------------------------------------------------------------------

class client_factory : public util::factory<client>
{
public:
    client_factory(boost::asio::io_context& context,
                   const client_factory_config& config);

    virtual std::unique_ptr<client> create() const override;

private:
    std::string m_hostname;
    uint16_t m_port;
    std::string m_client_version;

    boost::asio::io_context& m_context;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
