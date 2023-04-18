//
// Created by masi on 4/13/23.
//

#include "server_info.h"

namespace uh::net {

// ---------------------------------------------------------------------

server_info::server_info(const net::server &server) : m_server {server}
{

}

// ---------------------------------------------------------------------

bool server_info::server_busy() const
{
    return m_server.is_busy();
}

// ---------------------------------------------------------------------

} // namespace uh::net
