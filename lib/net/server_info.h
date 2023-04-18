//
// Created by masi on 4/12/23.
//

#ifndef CORE_SERVER_INFO_H
#define CORE_SERVER_INFO_H


#include "plain_server.h"

namespace uh::net {

// ---------------------------------------------------------------------

class server_info
{
    const net::server &m_server;
public:
    explicit server_info (const net::server &server);

    [[nodiscard]] bool server_busy() const;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif //CORE_SERVER_INFO_H
