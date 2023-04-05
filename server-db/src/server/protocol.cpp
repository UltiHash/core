#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <protocol/exception.h>

using namespace uh::protocol;

namespace uh::dbn::server
{

// ---------------------------------------------------------------------

protocol::protocol(storage::backend& storage)
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

std::unique_ptr<io::device> protocol::on_read_block(uh::protocol::blob&& hash)
{
    return m_storage.read_block(hash);
}

// ---------------------------------------------------------------------

std::size_t protocol::on_free_space()
{
    return m_storage.free_space();
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> protocol::on_allocate_chunk(std::size_t size)
{
    return m_storage.allocate(size);
}

// ---------------------------------------------------------------------

std::size_t protocol::on_next_chunk(std::span<char>)
{
    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::server
