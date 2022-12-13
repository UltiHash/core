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

blob client::write_chunk(const blob& data)
{
    write(m_io, write_chunk::request{ .content = std::move(data) });
    m_io.flush();

    write_chunk::response response;
    read(m_io, response);

    return std::move(response.hash);
}

// ---------------------------------------------------------------------

blob client::read_chunk(const blob& hash)
{
    write(m_io, read_chunk::request{ .hash = std::move(hash) });
    m_io.flush();

    read_chunk::response response;
    read(m_io, response);

    return std::move(response.content);
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
