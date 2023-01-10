#include "server.h"

#include "exception.h"
#include "messages.h"
#include "serializer.h"

#include <logging/logging_boost.h>


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

void server::on_reset()
{
}

// ---------------------------------------------------------------------

void server::handle(std::shared_ptr<net::socket> client)
{
    boost::iostreams::stream<net::socket_device> io(client);

    m_state = server_state::setup;

    while (io.is_open() && m_state != server_state::disconnected)
    {
        try
        {
            uint8_t request_id;
            read(io, request_id);

            switch (m_state)
            {
                case server_state::setup: handle_setup_request(io, request_id); break;
                case server_state::normal: handle_normal_request(io, request_id); break;
                case server_state::reading: handle_reading_request(io, request_id); break;

                default:
                    write(io, status{ .code = status::FAILED, .message = "unsupported state" });
                    io.close();
                    ERROR << "unsupported state";
                    return;
            }
        }
        catch (const read_error& e)
        {
            ERROR << e.what();
            break;
        }
        catch (const std::exception& e)
        {
            write(io, status{ .code = status::FAILED, .message = e.what() });
            io.flush();
            m_state = server_state::normal;
        }
    }
}

// ---------------------------------------------------------------------

void server::handle_setup_request(iostream& io, uint8_t request_id)
{
    switch (request_id)
    {
        case hello::request_id: return handle_hello(io);
        case quit::request_id: return handle_quit(io);

        default: throw std::runtime_error("unsupported command");
    }
}

// ---------------------------------------------------------------------

void server::handle_normal_request(iostream& io, uint8_t request_id)
{
    switch (request_id)
    {
        case write_block::request_id: return handle_write_block(io);
        case read_block::request_id: return handle_read_block(io);
        case quit::request_id: return handle_quit(io);
        case free_space::request_id: return handle_free_space(io);
        case reset::request_id: return handle_reset(io);

        default: throw std::runtime_error("unsupported command");
    }
}

// ---------------------------------------------------------------------

void server::handle_reading_request(iostream& io, uint8_t request_id)
{
    switch (request_id)
    {
        case quit::request_id: return handle_quit(io);
        case reset::request_id: return handle_reset(io);

        default: throw std::runtime_error("unsupported command");
    }
}

// ---------------------------------------------------------------------

void server::handle_hello(iostream& io)
{
    hello::request req;
    read(io, req);

    server_information info;

    try
    {
        info = on_hello(req.client_version);
    }
    catch (const std::exception& e)
    {
        write(io, status{ .code = status::FAILED, .message = e.what() });
        m_state = server_state::disconnected;
        return;
    }

    write(io, status{ status::OK });
    write(io, hello::response{
        .server_version = info.version,
        .protocol_version = info.protocol });
    io.flush();

    m_state = server_state::normal;
}

// ---------------------------------------------------------------------

void server::handle_write_block(iostream& io)
{
    write_block::request req;
    read(io, req);

    blob hash = on_write_block(std::move(req.content));

    m_state = server_state::normal;

    write(io, status{ status::OK });
    write(io, write_block::response{ std::move(hash) });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_read_block(iostream& io)
{
    read_block::request req;
    read(io, req);

    blob content = on_read_block(std::move(req.hash));

    m_state = server_state::normal;

    write(io, status{ status::OK });
    write(io, read_block::response{ std::move(content) });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_quit(iostream& io)
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

    m_state = server_state::disconnected;

    write(io, status{ status::OK });
    io.flush();
    io.close();
}

// ---------------------------------------------------------------------

void server::handle_free_space(iostream& io)
{
    free_space::request req;
    read(io, req);

    auto space = on_free_space();

    m_state = server_state::normal;

    write(io, status{ status::OK });
    write(io, free_space::response{ .space_available = space });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_reset(iostream& io)
{
    reset::request req;
    read(io, req);

    on_reset();

    m_state = server_state::normal;

    write(io, status{ status::OK });
    io.flush();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
