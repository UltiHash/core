#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <protocol/exception.h>
#include <util/exception.h>

#include <vector>

using namespace uh::protocol;

namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol::protocol(cluster::mod& cluster,
                   an::persistence::client_metrics& persistence,
                   metrics::client_metrics& client,
                   const uh::net::server_info &serv_info):
        m_cluster(cluster),
        m_persistence(persistence),
        m_client(client),
        m_serv_info (serv_info)
{
}

// ---------------------------------------------------------------------

uh::protocol::server_information protocol::on_hello(const std::string& client_version)
{

    if (m_serv_info.server_busy())
    {
        THROW(server_busy, "server is busy, try again later");
    }

    INFO << "connection from client with version " << client_version;

    return
    {
        .version = PROJECT_VERSION,
        .protocol = 1,
    };
}

// ---------------------------------------------------------------------

void protocol::on_client_statistics(uh::protocol::client_statistics::request& client_stat)
{
    // ! TODO: set_uhv_metrics should technically be in protocol_metrics_wrapper
    m_client.set_uhv_metrics(client_stat);
    m_persistence.add(client_stat);
}

// ---------------------------------------------------------------------

std::size_t protocol::on_free_space()
{
    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

uh::protocol::write_chunks::response protocol::on_write_chunks(const write_chunks::request &req) {
    return m_cluster.write_chunks (req);
}

// ---------------------------------------------------------------------

uh::protocol::read_chunks::response protocol::on_read_chunks(const read_chunks::request &req) {
    return m_cluster.read_chunks (req);
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
