#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>

#include <vector>


using namespace uh::protocol;

namespace uh::an
{

// ---------------------------------------------------------------------

protocol::protocol(uh::protocol::client_pool& clients)
    : m_clients(clients)
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

blob protocol::on_write_chunk(blob&& data)
{
    return m_clients.get()->write_chunk(data);
}

// ---------------------------------------------------------------------

blob protocol::on_read_chunk(blob&& hash)
{
    return m_clients.get()->read_chunk(hash);
}

// ---------------------------------------------------------------------

} // namespace uh::an
