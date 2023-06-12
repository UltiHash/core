#include "client.h"

#include "messages.h"

#include <util/exception.h>


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

    m_server_uuid = resp.server_uuid;

    return {
        .version = resp.server_version,
        .uuid = resp.server_uuid,
        .protocol = resp.protocol_version,
    };

}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> client::read_block(const blob& hash)
{
    THROW(util::exception, "client::read_block is disabled");
}

// ---------------------------------------------------------------------

std::unique_ptr<allocation> client::allocate(std::size_t size)
{
    THROW(util::exception, "client::allocate is disabled");
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
    THROW(util::exception, "client::finalize is disabled");
}

// ---------------------------------------------------------------------

std::streamsize client::write_chunk(std::span<const char> buffer)
{
    THROW(util::exception, "client::write_chunk is disabled");
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

uh::protocol::write_chunks::response client::write_chunks(const uh::protocol::write_chunks::request &req) {
    write (m_bs, req);
    m_bs.sync();

    uh::protocol::write_chunks::response resp;
    read (m_bs, resp);
    return resp;
}

// ---------------------------------------------------------------------

uh::protocol::read_chunks::response client::read_chunks (const read_chunks::request &req) {
    write (m_bs, req);
    m_bs.sync();
    uh::protocol::read_chunks::response resp;
    read (m_bs, resp);
    return resp;
}

// ---------------------------------------------------------------------

std::string& client::get_server_uuid() {
    return m_server_uuid;
}

} // namespace uh::protocol
