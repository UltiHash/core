#include "server.h"

#include "messages.h"
#include "serializer.h"

#include <logging/logging_boost.h>
#include <boost/iostreams/stream.hpp>


using namespace boost::asio;

namespace uh::protocol
{

// ---------------------------------------------------------------------

void server::handle(std::shared_ptr<net::socket> client)
{
    boost::iostreams::stream<net::socket_device> io(client);

    while (io)
    {
        uint8_t request_id;

        try
        {
            read(io, request_id);
        }
        catch (const std::exception& e)
        {
            ERROR << e.what();
        }

        try
        {
            switch (request_id)
            {
                case hello::request_id: handle_hello(io); break;
                case write_chunk::request_id: handle_write_chunk(io); break;
                case read_chunk::request_id: handle_read_chunk(io); break;
                default: throw std::runtime_error("unsupported command");
            }
        }
        catch (const std::exception& e)
        {
            write(io, status{ .code = status::OK, .message = e.what() });
            io.flush();
        }
    }
}

// ---------------------------------------------------------------------

void server::handle_hello(std::iostream& io)
{
    hello::request req;
    read(io, req);

    server_information info = on_hello(req.client_version);

    write(io, status{ status::OK });
    write(io, hello::response{
        .server_version = info.version,
        .protocol_version = info.protocol });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_write_chunk(std::iostream& io)
{
    write_chunk::request req;
    read(io, req);

    blob hash = on_write_chunk(std::move(req.content));

    write(io, status{ status::OK });
    write(io, write_chunk::response{ std::move(hash) });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_read_chunk(std::iostream& io)
{
    read_chunk::request req;
    read(io, req);

    blob content = on_read_chunk(std::move(req.hash));

    write(io, status{ status::OK });
    write(io, read_chunk::response{ std::move(content) });
    io.flush();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
