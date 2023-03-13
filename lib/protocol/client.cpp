#include "client.h"

#include "client_allocation.h"
#include "read_block_device.h"
#include "messages.h"


using namespace uh::io;
using namespace uh::net;

namespace uh::protocol
{

// ---------------------------------------------------------------------

client::client(std::unique_ptr<net::socket> sock)
    : m_sock(std::move(sock)),
      m_io(boost_device(m_sock))
{
}

// ---------------------------------------------------------------------

client::~client()
{
    if (m_io)
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
    write(m_io, hello::request{ .client_version = client_version });
    m_io.flush();

    hello::response resp;
    read(m_io, resp);

    return {
        .version = resp.server_version,
        .protocol = resp.protocol_version,
    };
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> client::read_block(const blob& hash)
{
    write(m_io, read_block::request{ .hash = std::move(hash) });
    m_io.flush();

    read_block::response response;
    read(m_io, response);

    return std::make_unique<read_block_device>(*this);
}

// ---------------------------------------------------------------------

std::unique_ptr<allocation> client::allocate(std::size_t size)
{
    write(m_io, allocate_chunk::request{ .size = size });
    m_io.flush();

    allocate_chunk::response response;
    read(m_io, response);

    return std::make_unique<client_allocation>(*this);
}

// ---------------------------------------------------------------------

void client::quit(const std::string& reason)
{
    write(m_io, quit::request{ .reason = reason });
    m_io.flush();

    quit::response response;
    read(m_io, response);
}

// ---------------------------------------------------------------------

std::size_t client::free_space()
{
    write(m_io, free_space::request{});
    m_io.flush();

    free_space::response response;
    read(m_io, response);

    return response.space_available;
}

// ---------------------------------------------------------------------

void client::reset()
{
    write(m_io, reset::request{});
    m_io.flush();

    reset::response response;
    read(m_io, response);
}

// ---------------------------------------------------------------------

std::streamsize client::next_chunk(std::span<char> buffer)
{
    write(m_io, next_chunk::request{ .max_size = static_cast<uint32_t>(buffer.size()) });
    m_io.flush();

    next_chunk::response response{ .content = buffer };
    read(m_io, response);

    return response.content.size();
}

// ---------------------------------------------------------------------

block_meta_data client::finalize()
{
    write(m_io, finalize_block::request{});
    m_io.flush();

    finalize_block::response response;
    read(m_io, response);

    return { std::move(response.hash), response.effective_size };
}

// ---------------------------------------------------------------------

std::streamsize client::write_chunk(std::span<const char> buffer)
{
    std::span<char> non_const(const_cast<char*>(buffer.data()), buffer.size());

    write(m_io, write_chunk::request{ .data = non_const });
    m_io.flush();

    write_chunk::response response;
    read(m_io, response);

    return buffer.size();
}

// ---------------------------------------------------------------------

bool client::valid() const
{
    return m_sock->valid();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
