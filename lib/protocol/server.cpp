#include "server.h"

#include "exception.h"
#include "messages.h"
#include "serializer.h"

#include <logging/logging_boost.h>
#include <boost/iostreams/stream.hpp>


using namespace boost::asio;

namespace uh::protocol
{

// ---------------------------------------------------------------------

std::size_t server::on_free_space()
{
    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

void server::on_quit(const std::string&)
{
}

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
                case write_block::request_id: handle_write_block(io); break;
                case read_block::request_id: handle_read_block(io); break;
                case free_space::request_id: handle_free_space(io); break;

                case quit::request_id:
                    handle_quit(io);
                    io.close();
                    return;

                default: throw std::runtime_error("unsupported command");
            }
        }
        catch (const std::exception& e)
        {
            write(io, status{ .code = status::FAILED, .message = e.what() });
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

void server::handle_write_block(std::iostream& io)
{
    write_block::request req;
    read(io, req);

    blob hash = on_write_block(std::move(req.content));

    write(io, status{ status::OK });
    write(io, write_block::response{ std::move(hash) });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_read_block(std::iostream& io)
{
    read_block::request req;
    read(io, req);

    blob content = on_read_block(std::move(req.hash));

    write(io, status{ status::OK });
    write(io, read_block::response{ std::move(content) });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_quit(std::iostream& io)
{
    quit::request req;
    read(io, req);

    try
    {
        on_quit(req.reason);
    }
    catch (...)
    {
        // ignore
    }

    write(io, status{ status::OK });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_free_space(std::iostream& io)
{
    free_space::request req;
    read(io, req);

    auto space = on_free_space();

    write(io, status{ status::OK });
    write(io, free_space::response{ .space_available = space });
    io.flush();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
