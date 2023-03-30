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

std::size_t server::on_next_chunk(std::span<char>)
{
    THROW(unsupported, "this call is not supported by this node type");
}

// ---------------------------------------------------------------------

void server::on_finalize()
{
}

// ---------------------------------------------------------------------

void server::on_write_chunk(std::span<char>)
{
}

// ---------------------------------------------------------------------

void server::handle(std::shared_ptr<net::socket> client)
{
    boost::iostreams::stream<io::boost_device> io(client);

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
                case server_state::writing: handle_writing_request(io, request_id); break;

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
            m_read_block.reset();
            m_write_alloc.reset();
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

        default:
            throw std::runtime_error("setup, unsupported command: "
                + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_normal_request(iostream& io, uint8_t request_id)
{
    switch (request_id)
    {
        case read_block::request_id: return handle_read_block(io);
        case quit::request_id: return handle_quit(io);
        case free_space::request_id: return handle_free_space(io);
        case reset::request_id: return handle_reset(io);
        case allocate_chunk::request_id: return handle_allocate_chunk(io);

        default:
            throw std::runtime_error("normal, unsupported command: "
                + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_reading_request(iostream& io, uint8_t request_id)
{
    switch (request_id)
    {
        case quit::request_id: return handle_quit(io);
        case reset::request_id: return handle_reset(io);
        case next_chunk::request_id: return handle_next_chunk(io);

        default:
            throw std::runtime_error("reading, unsupported command: "
                + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_writing_request(iostream& io, uint8_t request_id)
{
    switch (request_id)
    {
        case quit::request_id: return handle_quit(io);
        case reset::request_id: return handle_reset(io);
        case write_chunk::request_id: return handle_write_chunk(io);
        case finalize_block::request_id: return handle_finalize_block(io);

        default:
            throw std::runtime_error("writing, unsupported command: "
                + std::to_string(request_id));
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

void server::handle_read_block(iostream& io)
{
    read_block::request req;
    read(io, req);

    m_read_block = on_read_block(std::move(req.hash));

    m_state = server_state::reading;

    write(io, status{ status::OK });
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
    m_read_block.reset();
    m_write_alloc.reset();

    write(io, status{ status::OK });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_next_chunk(iostream& io)
{
    next_chunk::request req;
    read(io, req);

    if (req.max_size < MINIMUM_CHUNK_SIZE || req.max_size > MAXIMUM_CHUNK_SIZE)
    {
        THROW(illegal_args, "buffer size out of range");
    }

    std::vector<char> buffer(req.max_size);
    auto count = m_read_block->read(std::span<char>(buffer.begin(), buffer.end()));
    if (count == 0)
    {
        m_state = server_state::normal;
        m_read_block.reset();
    }

    write(io, status{ status::OK });
    write(io, next_chunk::response{ .content = std::span<char>(buffer.begin(), count) });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_allocate_chunk(iostream& io)
{
    allocate_chunk::request req;
    read(io, req);

    if (req.size > MAXIMUM_BLOCK_SIZE)
    {
        THROW(illegal_args, "block size out of range");
    }

    m_write_alloc = on_allocate_chunk(req.size);
    m_state = server_state::writing;

    write(io, status{ status::OK });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_write_chunk(iostream& io)
{
    std::vector<char> buffer(MAXIMUM_CHUNK_SIZE);
    write_chunk::request req{ .data = buffer };
    read(io, req);

    if (!m_write_alloc)
    {
        THROW(internal_error, "no space allocated");
    }

    on_write_chunk(req.data);
    m_write_alloc->device().write(req.data);
    write(io, status{ status::OK });
    io.flush();
}

// ---------------------------------------------------------------------

void server::handle_finalize_block(iostream& io)
{
    finalize_block::request req;
    read(io, req);

    if (!m_write_alloc)
    {
        THROW(internal_error, "no space allocated");
    }

    on_finalize();

    auto meta_data = m_write_alloc->persist();
    m_write_alloc.reset();

    write(io, status{ status::OK });
    write(io, finalize_block::response{
        .hash = std::move(meta_data.hash),
        .effective_size = meta_data.effective_size });
    io.flush();

    m_state = server_state::normal;
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
