#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <util/exception.h>

#include <vector>


using namespace uh::protocol;

namespace uh::an::server
{

// ---------------------------------------------------------------------

protocol::protocol(cluster::mod& cluster, const metrics::metrics& metrics)
    : m_cluster(cluster),
      m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

server_information protocol::on_hello(const std::string& client_version)
{
    INFO << "connection from client with version " << client_version;

    m_metrics.reqs_hello().Increment();

    return {
        .version = PROJECT_VERSION,
        .protocol = 1,
    };
}

// ---------------------------------------------------------------------

blob protocol::on_write_chunk(blob&& data)
{
    m_metrics.reqs_write_chunk().Increment();

    auto free_space = m_cluster.bc_free_space();
    if (free_space.empty())
    {
        THROW(util::exception, "no storage back-end configured");
    }

    free_space.sort([](auto& l, auto& r) { return l.second > r.second; });

    auto node_ref = free_space.front().first;

    return m_cluster.node(node_ref).get()->write_chunk(data);
}

// ---------------------------------------------------------------------

blob protocol::on_read_chunk(blob&& hash)
{
    m_metrics.reqs_read_chunk().Increment();

    return m_cluster.bc_read_chunk(hash);
}

// ---------------------------------------------------------------------

} // namespace uh::an::server
