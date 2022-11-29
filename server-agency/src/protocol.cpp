#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>

#include <vector>


using namespace uh::protocol;

namespace uh::an
{

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
    throw std::runtime_error("to be implemented");
}

// ---------------------------------------------------------------------

blob protocol::on_read_chunk(blob&& hash)
{
    throw std::runtime_error("to be implemented");
}

// ---------------------------------------------------------------------

} // namespace uh::an
