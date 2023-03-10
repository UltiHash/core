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

class read_block_device;

// ---------------------------------------------------------------------

class client
{
public:
    client(std::unique_ptr<net::socket> sock);

    ~client();

    /**
     * Send client version information to the server. This must be the first
     * command in a client session. The server returns it's version and the
     * version of the supported protocol.
     *
     * @throw on error status
     */
    server_information hello(const std::string& client_version);

    /**
     * Send a `write_block` request to the server. The block will be stored
     * in the UltiHash storage back-end. The server returns a hash that can
     * be used to identify the block in the cloud and the effective space
     * used to store the block.
     *
     * @throw on error status
     */
    std::pair<blob, std::size_t> write_block(const blob& data);

    /**
     * Send a `read_block` request to the server. The server will look up
     * the hash and return the associated data, if available.
     *
     * @return a device that can be used to read the block. If the device is
     *         destructed the reading will be stopped. While the device is
     *         alive one must not call other client functions. The device becomes
     *         invalid when the client is destructed.
     * @throw on error status
     */
    std::unique_ptr<io::device> read_block(const blob& hash);

    /**
     * End the connection by sending the `quit` command, optionally with a
     * reason.
     */
    void quit(const std::string& reason);

    /**
     * Returns free space on the peer in bytes.
     */
    std::size_t free_space();

    /**
     * Reset the connection to normal state.
     */
    void reset();

    /**
     * Return true, if there is a working underlying connection. False indicates
     * that there was an I/O error before.
     */
    bool valid() const;

private:
    friend class read_block_device;

    /**
     * Request next chunk from remote. This function is to be called by read_block_device only.
     */
    std::streamsize next_chunk(std::span<char> buffer);

    std::shared_ptr<net::socket> m_sock;
    boost::iostreams::stream<io::boost_device> m_io;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
