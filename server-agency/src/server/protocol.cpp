#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <util/exception.h>

#include <vector>


using namespace uh::protocol;

namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol::protocol(cluster::mod& cluster)
    : m_cluster(cluster)
{
}

// ---------------------------------------------------------------------

server_information protocol::on_hello(const std::string& client_version)
{
    INFO << "connection from client with version " << client_version;

    return {
        .version = PROJECT_VERSION,
        .protocol = 1,
    };
}

// ---------------------------------------------------------------------

blob protocol::on_write_block(blob&& data)
{
    auto free_space = m_cluster.bc_free_space();
    if (free_space.empty())
    {
        THROW(util::exception, "no storage back-end configured");
    }

    free_space.sort([](auto& l, auto& r) { return l.second > r.second; });

    auto node_ref = free_space.front().first;

    return m_cluster.node(node_ref).get()->write_block(data);
}

// ---------------------------------------------------------------------

blob protocol::on_read_block(blob&& hash)
{
    return m_cluster.bc_read_block(hash);
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
