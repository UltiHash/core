#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>


using namespace uh::protocol;

namespace uh::dbn
{

// ---------------------------------------------------------------------

protocol::protocol(storage_backend& storage)
    : m_storage(storage)
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
    return m_storage.write_block(data);
}

// ---------------------------------------------------------------------

blob protocol::on_read_chunk(blob&& hash)
{
    return m_storage.read_block(hash);
}

// ---------------------------------------------------------------------

} // namespace uh::dbn
