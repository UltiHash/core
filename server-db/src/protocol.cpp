#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>


using namespace uh::protocol;

namespace uh::dbn
{

// ---------------------------------------------------------------------

protocol::protocol(storage_backend& storage, const metrics& metrics)
    : m_storage(storage),
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
    return m_storage.write_block(data);
}

// ---------------------------------------------------------------------

blob protocol::on_read_chunk(blob&& hash)
{
    m_metrics.reqs_read_chunk().Increment();
    return m_storage.read_block(hash);
}

// ---------------------------------------------------------------------

} // namespace uh::dbn
