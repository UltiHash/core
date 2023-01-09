#ifndef SERVER_AGENCY_SERVER_PROTOCOL_H
#define SERVER_AGENCY_SERVER_PROTOCOL_H

#include <cluster/mod.h>
#include <protocol/client_pool.h>
#include <protocol/server.h>
#include <boost/asio.hpp>

#include <metrics/metrics.h>

#include <memory>


using namespace boost::asio::ip;

namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::server
{
public:
    protocol(cluster::mod& cluster, const metrics::metrics& metrics);

    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual uh::protocol::blob on_write_chunk(uh::protocol::blob&& data) override;
    virtual uh::protocol::blob on_read_chunk(uh::protocol::blob&& hash) override;

private:
    cluster::mod& m_cluster;
    const metrics::metrics& m_metrics;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
