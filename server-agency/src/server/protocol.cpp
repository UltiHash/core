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

protocol::protocol(cluster::mod& cluster, const uh::net::server_info &serv_info):
        m_cluster(cluster),
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

} // namespace uh::an::server
