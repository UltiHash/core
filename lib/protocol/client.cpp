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
          m_bs{*m_sock}
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
    write(m_bs, hello::request{ .client_version = client_version });
    m_bs.sync();

    hello::response resp;
    read(m_bs, resp);

    return {
        .version = resp.server_version,
        .protocol = resp.protocol_version,
    };
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> client::read_block(const blob& hash)
{
    write(m_bs, read_block::request{ .hash = std::move(hash) });
    m_bs.sync();

    read_block::response response;
    read(m_bs, response);

    return std::make_unique<read_block_device>(*this);
}

// ---------------------------------------------------------------------

std::unique_ptr<allocation> client::allocate(std::size_t size)
{
    write(m_bs, allocate_chunk::request{ .size = size });
    m_bs.sync();

    allocate_chunk::response response;
    read(m_bs, response);

    return std::make_unique<client_allocation>(*this);
}

// ---------------------------------------------------------------------

protocol::block_meta_data client::write_small_block(std::span <char> data) {

    write(m_bs, write_small_block::request{.data = data});
    m_bs.sync();

    write_small_block::response response;
    read(m_bs, response);

    return {response.hash, response.effective_size};
}

// ---------------------------------------------------------------------

void client::quit(const std::string& reason)
{
    write(m_bs, quit::request{ .reason = reason });
    m_bs.sync();

    quit::response response;
    read(m_bs, response);
}

// ---------------------------------------------------------------------

std::size_t client::free_space()
{
    write(m_bs, free_space::request{});
    m_bs.sync();

    free_space::response response;
    read(m_bs, response);

    return response.space_available;
}

// ---------------------------------------------------------------------

void client::reset()
{
    write(m_bs, reset::request{});
    m_bs.sync();

    reset::response response;
    read(m_bs, response);
}

// ---------------------------------------------------------------------

std::streamsize client::next_chunk(std::span<char> buffer)
{
    write(m_bs, next_chunk::request{ .max_size = static_cast<uint32_t>(buffer.size()) });
    m_bs.sync();

    next_chunk::response response{ .content = buffer };
    read(m_bs, response);

    return response.content.size();
}

// ---------------------------------------------------------------------

block_meta_data client::finalize()
{
    write(m_bs, finalize_block::request{});
    m_bs.sync();

    finalize_block::response response;
    read(m_bs, response);

    return { std::move(response.hash), response.effective_size };
}

// ---------------------------------------------------------------------

std::streamsize client::write_chunk(std::span<const char> buffer)
{
    std::span<char> non_const(const_cast<char*>(buffer.data()), buffer.size());

    write(m_bs, write_chunk::request{ .data = non_const });
    m_bs.sync();

    write_chunk::response response;
    read(m_bs, response);

    return buffer.size();
}

// ---------------------------------------------------------------------

bool client::valid() const
{
    return m_sock->valid();
}

// ---------------------------------------------------------------------

void client::send_client_statistics(const uh::protocol::client_statistics::request& client_stat)
{
    write(m_bs, client_stat);
    m_bs.sync();

    client_statistics::response response;
    read(m_bs, response);
}

// ---------------------------------------------------------------------

protocol::write_chunks::response client::write_chunks(const write_chunks::request &req) {
    write (m_bs, req);
    m_bs.sync();

    write_chunks::response resp;
    read (m_bs, resp);
    return resp;
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
