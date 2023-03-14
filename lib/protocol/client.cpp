#include "client.h"

#include "read_block_device.h"
#include "messages.h"


using namespace uh::io;
using namespace uh::net;

namespace uh::protocol
{

// ---------------------------------------------------------------------

client::client(std::unique_ptr<net::socket> sock)
    : m_sock(std::move(sock)),
      bs{*m_sock}
{
}

// ---------------------------------------------------------------------

client::~client()
{
    if (m_sock->valid())
    {
        try
        {
            quit("terminated");
        }
        catch (...)
        {
            // ignore
        }
    }
}

// ---------------------------------------------------------------------

server_information client::hello(const std::string& client_version)
{
    write(bs, hello::request{ .client_version = client_version });
    bs.sync();
    
    hello::response resp;
    read(bs, resp);

    return {
        .version = resp.server_version,
        .protocol = resp.protocol_version,
    };
}

// ---------------------------------------------------------------------

block_meta_data client::write_block(const blob& data)
{
    write(bs, write_block::request{ .content = std::move(data) });
    bs.sync();

    write_block::response response;
    read(bs, response);

    return {
        .hash = std::move(response.hash),
        .effective_size = response.effective_size,
    };
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> client::read_block(const blob& hash)
{
    write(bs, read_block::request{ .hash = std::move(hash) });
    bs.sync();

    read_block::response response;
    read(bs, response);

    return std::make_unique<read_block_device>(*this);
}

// ---------------------------------------------------------------------

void client::quit(const std::string& reason)
{
    write(bs, quit::request{ .reason = reason });
    bs.sync();

    quit::response response;
    read(bs, response);
}

// ---------------------------------------------------------------------

std::size_t client::free_space()
{
    write(bs, free_space::request{});
    bs.sync();

    free_space::response response;
    read(bs, response);

    return response.space_available;
}

// ---------------------------------------------------------------------

void client::reset()
{
    write(bs, reset::request{});
    bs.sync();

    reset::response response;
    read(bs, response);
}

// ---------------------------------------------------------------------

std::streamsize client::next_chunk(std::span<char> buffer)
{
    write(bs, next_chunk::request{ .max_size = static_cast<uint32_t>(buffer.size()) });
    bs.sync();

    next_chunk::response response{ .content = buffer };
    read(bs, response);

    return response.content.size();
}

// ---------------------------------------------------------------------

bool client::valid() const
{
    return m_sock->valid();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
