#ifndef PROTOCOL_REQUEST_INTERFACE_H
#define PROTOCOL_REQUEST_INTERFACE_H

#include "protocol.h"
#include "allocation.h"

namespace uh::protocol
{

// ---------------------------------------------------------------------

struct request_interface
{

    virtual uh::protocol::server_information on_hello(const std::string &client_version) = 0;

    virtual std::unique_ptr <io::device> on_read_block(uh::protocol::blob &&hash) = 0;

    virtual std::size_t on_free_space() = 0;

    virtual void on_quit(const std::string &reason) = 0;

    virtual void on_reset() = 0;

    virtual std::size_t on_next_chunk(std::span<char> buffer) = 0;

    virtual void on_finalize() = 0;

    virtual void on_write_chunk(std::span<char> buffer) = 0;

    virtual std::unique_ptr <uh::protocol::allocation> on_allocate_chunk(std::size_t size) = 0;

    virtual ~request_interface() = default;

};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
