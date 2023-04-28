#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include <net/socket.h>
#include "allocation.h"
#include "common.h"
#include "serialization/serialization.h"
#include "messages.h"

#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <string>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class client_allocation;
class read_block_device;
class write_block_device;

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
     * Allocate space for a given chunk
     */
    std::unique_ptr<allocation> allocate(std::size_t);

    /**
     * Sends the data to the data node and returns the hash and the effective size of the data.
     * If the data node does not have free space it returns a failed status.
     *
     * @param data actual data to be integrated
     * @return hash and effective size of the integrated data
     */
    block_meta_data write_small_block(std::span <char> data);

    /**
     * Sends several blocks at once to the data node and returns the hashes and the effective sizes of the blocks.
     * If the data node does not have free space it returns a failed status.
     *
     * @param data several blocks to be integrated
     * @return hashes and effective sizes of the integrated data
     */
    uh::protocol::write_xsmall_blocks::response write_xsmall_blocks (const uh::protocol::write_xsmall_blocks::request &);

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
    friend class client_allocation;
    friend class read_block_device;
    friend class write_block_device;

    /**
     * Request next chunk from remote. This function is to be called by read_block_device only.
     */
    std::streamsize next_chunk(std::span<char> buffer);

    /**
     * Finalize chunk upload and end allocation.
     */
    block_meta_data finalize();

    /**
     * Send a chunk of the currently written block.
     */
    std::streamsize write_chunk(std::span<const char> buffer);

    std::shared_ptr<net::socket> m_sock;
    serialization::buffered_serialization m_bs;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
