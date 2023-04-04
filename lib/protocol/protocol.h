#ifndef PROTOCOL_PROTOCOL_H
#define PROTOCOL_PROTOCOL_H

#include <net/socket.h>
#include <memory>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class protocol
{
protected:
    std::shared_ptr<net::socket> client_;
public:
    explicit protocol(std::shared_ptr<net::socket> client): client_(std::move(client)) {}

    virtual ~protocol() = default;
    virtual void handle() = 0;
};

// ---------------------------------------------------------------------

class protocol_factory
{
public:
    virtual ~protocol_factory() = default;
    virtual std::unique_ptr<uh::protocol::protocol> create(std::shared_ptr<net::socket> client) = 0;
};

// ---------------------------------------------------------------------

struct handler_interface
{

    virtual uh::protocol::server_information on_hello(const std::string& client_version) = 0;
    virtual std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) = 0;
    virtual std::size_t on_free_space() = 0;

    virtual void on_quit(const std::string& reason) = 0;
    virtual void on_reset() = 0;
    virtual std::size_t on_next_chunk(std::span<char> buffer) = 0;

    virtual void on_finalize() = 0;
    virtual void on_write_chunk(std::span<char> buffer) = 0;
    virtual std::unique_ptr<uh::protocol::allocation> on_allocate_chunk(std::size_t size) = 0;

    virtual ~handler_interface() = default;

};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
