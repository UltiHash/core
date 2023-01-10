#include "client.h"

#include "messages.h"


using namespace uh::net;

namespace uh::protocol
{

// ---------------------------------------------------------------------

client::client(std::unique_ptr<net::socket> sock)
    : m_sock(std::move(sock)),
      m_io(socket_device(m_sock))
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

blob client::write_block(const blob& data)
{
    write(m_io, write_block::request{ .content = std::move(data) });
    m_io.flush();

    write_block::response response;
    read(m_io, response);

    return std::move(response.hash);
}

// ---------------------------------------------------------------------

blob client::read_block(const blob& hash)
{
    write(m_io, read_block::request{ .hash = std::move(hash) });
    m_io.flush();

    read_block::response response;
    read(m_io, response);

    return std::move(response.content);
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

bool client::valid() const
{
    return m_sock->valid();;
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
