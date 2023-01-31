#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <util/exception.h>

#include <vector>


using namespace uh::protocol;

namespace uh::dbn::server
{

// ---------------------------------------------------------------------

protocol::protocol(storage::mod& storage)
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
    auto free_space = m_storage.free_space();
    if (free_space == 0)
    {
        THROW(util::exception, "No free space on storage backend");
    }

    return m_storage.write_chunk(data);
}

// ---------------------------------------------------------------------

blob protocol::on_read_chunk(blob&& hash)
{
    return m_storage.read_chunk(hash);
}

// ---------------------------------------------------------------------

std::size_t protocol::on_free_space(){
    return m_storage.free_space();
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::server
