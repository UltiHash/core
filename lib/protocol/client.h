#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include <net/socket.h>
#include "common.h"

#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <string>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class client
{
public:
    client(std::unique_ptr<net::socket> sock);

    /**
     * Send client version information to the server. This must be the first
     * command in a client session. The server returns it's version and the
     * version of the supported protocol.
     *
     * @throw on error status
     */
    server_information hello(const std::string& client_version);

    /**
     * Send a `write_chunk` request to the server. The chunk will be stored
     * in the UltiHash storage back-end. The server returns a hash that can
     * be used to identify the chunk in the cloud.
     *
     * @throw on error status
     */
    blob write_chunk(const blob& data);

    /**
     * Send a `read_chunk` request to the server. The server will look up
     * the hash and return the associated data, if available.
     *
     * @throw on error status
     */
    blob read_chunk(const blob& hash);

private:
    std::shared_ptr<net::socket> m_sock;
    boost::iostreams::stream<net::socket_device> m_io;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
