#ifndef SERVER_AGENCY_PROTOCOL_H
#define SERVER_AGENCY_PROTOCOL_H

#include <protocol/client_pool.h>
#include <protocol/server.h>
#include <boost/asio.hpp>

#include "metrics.h"

#include <memory>


using namespace boost::asio::ip;

namespace uh::an
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::server
{
public:
    protocol(uh::protocol::client_pool& clients, const metrics& metrics);

    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual uh::protocol::blob on_write_chunk(uh::protocol::blob&& data) override;
    virtual uh::protocol::blob on_read_chunk(uh::protocol::blob&& hash) override;

private:
    uh::protocol::client_pool& m_clients;
    const metrics& m_metrics;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
