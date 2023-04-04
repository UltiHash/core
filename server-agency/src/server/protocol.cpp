#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <util/exception.h>

#include <vector>


namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol::protocol(cluster::mod& cluster):
        m_cluster(cluster)
{
}

// ---------------------------------------------------------------------

uh::protocol::server_information protocol::on_hello(const std::string& client_version)
{
    INFO << "connection from client with version " << client_version;

    return {
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

std::size_t protocol::on_free_space()
{
//    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

void protocol::on_quit(const std::string&)
{
}

// ---------------------------------------------------------------------

void protocol::on_reset()
{
}

// ---------------------------------------------------------------------

std::size_t protocol::on_next_chunk(std::span<char>)
{
//    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

void protocol::on_finalize()
{
}

// ---------------------------------------------------------------------

void protocol::on_write_chunk(std::span<char>)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> protocol::on_allocate_chunk(std::size_t size)
{
    return m_cluster.allocate(size);
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
