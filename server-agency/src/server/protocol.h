#ifndef SERVER_AGENCY_SERVER_PROTOCOL_H
#define SERVER_AGENCY_SERVER_PROTOCOL_H

#include <net/server_info.h>
#include <protocol/protocol.h>
#include <protocol/request_interface.h>

#include <cluster/mod.h>
#include <metrics/client_metrics.h>
#include <persistence/storage/client_metrics_persistence.h>

#include <memory>


using namespace boost::asio::ip;

namespace uh::an::server
{

// ---------------------------------------------------------------------

class protocol : public uh::protocol::request_interface
{
public:
    protocol(cluster::mod& cluster,
                      metrics::client_metrics& client,
                      const uh::net::server_info &serv_info);

    uh::protocol::server_information on_hello(const std::string& client_version) override;
    void on_client_statistics(uh::protocol::client_statistics::request& client_stat) override;
    uh::protocol::write_chunks::response on_write_chunks (const uh::protocol::write_chunks::request &) override;
    uh::protocol::read_chunks::response on_read_chunks (const uh::protocol::read_chunks::request &) override;

    std::size_t on_free_space() override;

private:
    cluster::mod& m_cluster;
    metrics::client_metrics& m_client;
    const uh::net::server_info &m_serv_info;
};

// ---------------------------------------------------------------------

} // namespace uh::an::server

#endif
