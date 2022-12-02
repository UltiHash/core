#include "client.h"

#include "messages.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

client::client(std::unique_ptr<net::connection> conn)
    : m_conn(std::move(conn)),
      m_io(std::move(m_conn->socket()))
{
}

// ---------------------------------------------------------------------

server_information client::hello(const std::string& client_version)
{
    write(m_io, hello::request{ .client_version = client_version });

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

    write_chunk::response response;
    read(m_io, response);

    return std::move(response.hash);
}

// ---------------------------------------------------------------------

blob client::read_chunk(const blob& hash)
{
    write(m_io, read_chunk::request{ .hash = std::move(hash) });

    read_chunk::response response;
    read(m_io, response);

    return std::move(response.content);
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
