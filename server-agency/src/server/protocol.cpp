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

std::unique_ptr<io::device> protocol::on_read_block(uh::protocol::blob&& hash)
{
    return m_cluster.bc_read_block(hash);
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> protocol::on_allocate_chunk(std::size_t size)
{
    return m_cluster.allocate(size);
}

// ---------------------------------------------------------------------

block_meta_data protocol::on_write_small_block (std::span <char> buffer)
{
    return m_cluster.write_small_block (buffer);
}

// ---------------------------------------------------------------------

void protocol::on_client_statistics(uh::protocol::client_statistics::request& client_stat)
{
    // ! TODO: set_uhv_metrics should technically be in protocol_metrics_wrapper
    m_client.set_uhv_metrics(client_stat);
    m_persistence.add(client_stat);
    // TODO: flush occasionally
    m_persistence.flush();
}

// ---------------------------------------------------------------------

std::size_t protocol::on_free_space()
{
    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

void protocol::on_next_chunk(std::span<char>)
{
    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

uh::protocol::write_xsmall_blocks::response
protocol::on_write_xsmall_blocks(const uh::protocol::write_xsmall_blocks::request &req)
{
    return m_cluster.write_xsmall_blocks (req);
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
