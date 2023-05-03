#ifndef PROTOCOL_REQUEST_INTERFACE_H
#define PROTOCOL_REQUEST_INTERFACE_H

#include "protocol.h"
#include "allocation.h"
#include "messages.h"

class reponse;
namespace uh::protocol
{

// ---------------------------------------------------------------------

struct request_interface
{

    virtual uh::protocol::server_information on_hello(const std::string& client_version) = 0;

    virtual std::unique_ptr <io::device> on_read_block(uh::protocol::blob &&hash) = 0;

    virtual std::unique_ptr <uh::protocol::allocation> on_allocate_chunk(std::size_t size) = 0;

    virtual block_meta_data on_write_small_block (std::span<char> buffer) = 0;

    virtual write_xsmall_blocks::response on_write_xsmall_blocks (const write_xsmall_blocks::request &) = 0;

    // default implementations

    virtual std::size_t on_free_space() { return 0; }

    virtual void on_quit(const std::string &reason) { };

    virtual void on_reset() { };

    virtual void on_next_chunk(std::span<char> buffer) { };

    virtual void on_finalize() { };

    virtual void on_write_chunk(std::span<char> buffer) { };

    virtual void on_client_statistics(uh::protocol::client_statistics::request& client_stat) {};

    virtual ~request_interface() = default;

};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
