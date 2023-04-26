#include "server.h"

#include "exception.h"
#include "messages.h"

#include <logging/logging_boost.h>


using namespace boost::asio;

namespace uh::protocol
{

// ---------------------------------------------------------------------

void server::handle()
{

    m_state = server_state::setup;

    while (client_->valid() && m_state != server_state::disconnected)
    {
        try
        {
            const auto request_id = m_bs.read <uint8_t> ();

            switch (m_state)
            {
                case server_state::setup: handle_setup_request(request_id); break;
                case server_state::normal: handle_normal_request(request_id); break;
                case server_state::reading: handle_reading_request(request_id); break;
                case server_state::writing: handle_writing_request(request_id); break;

                default:
                    write(m_bs, status{ .code = status::FAILED, .message = "unsupported state" });
                    m_bs.sync ();
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
            write(m_bs, status{ .code = status::FAILED, .message = e.what() });
            m_bs.sync ();
            m_read_block.reset();
            m_write_alloc.reset();
            m_state = server_state::normal;
        }
    }
}

// ---------------------------------------------------------------------

void server::handle_setup_request(uint8_t request_id)
{
    switch (request_id)
    {
        case hello::request_id: return handle_hello();
        case quit::request_id: return handle_quit();

        default:
            throw std::runtime_error("setup, unsupported command: "
                                     + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_normal_request(uint8_t request_id)
{
    switch (request_id)
    {
        case read_block::request_id: return handle_read_block();
        case quit::request_id: return handle_quit();
        case free_space::request_id: return handle_free_space();
        case reset::request_id: return handle_reset();
        case allocate_chunk::request_id: return handle_allocate_chunk();
        case write_small_block::request_id: return handle_write_small_block();
        case read_small_block::request_id: return handle_read_small_block();
        case client_statistics::request_id: return handle_client_statistics();

        default:
            throw std::runtime_error("normal, unsupported command: "
                                     + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_reading_request(uint8_t request_id)
{
    switch (request_id)
    {
        case quit::request_id: return handle_quit();
        case reset::request_id: return handle_reset();
        case next_chunk::request_id: return handle_next_chunk();

        default:
            throw std::runtime_error("reading, unsupported command: "
                                     + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_writing_request(uint8_t request_id)
{
    switch (request_id)
    {
        case quit::request_id: return handle_quit();
        case reset::request_id: return handle_reset();
        case write_chunk::request_id: return handle_write_chunk();
        case finalize_block::request_id: return handle_finalize_block();

        default:
            throw std::runtime_error("writing, unsupported command: "
                                     + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_hello()
{
    DEBUG << "hello request on " << client_->peer();
    hello::request req;
    read(m_bs, req);

    server_information info;

    try
    {
        info = m_handler_interface->on_hello(req.client_version);
    }
    catch (const std::exception& e)
    {
        write(m_bs, status{ .code = status::FAILED, .message = e.what() });
        m_bs.sync ();
        m_state = server_state::disconnected;
        return;
    }

    write(m_bs, status{ status::OK });
    write(m_bs, hello::response{
            .server_version = info.version,
            .protocol_version = info.protocol });
    m_bs.sync();

    m_state = server_state::normal;
}

// ---------------------------------------------------------------------

void server::handle_read_block()
{
    DEBUG << "read_block request on " << client_->peer();

    read_block::request req;
    read(m_bs, req);

    m_read_block = m_handler_interface->on_read_block(std::move(req.hash));

    m_state = server_state::reading;

    write(m_bs, status{ status::OK });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_quit()
{
    DEBUG << "quit request on " << client_->peer();

    quit::request req;
    read(m_bs, req);

    try
    {
        m_handler_interface->on_quit(req.reason);
    }
    catch (...)
    {
        // ignore
    }

    m_state = server_state::disconnected;

    write(m_bs, status{ status::OK });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_free_space()
{
    DEBUG << "free_space request on " << client_->peer();

    free_space::request req;
    read(m_bs, req);

    auto space = m_handler_interface->on_free_space();

    m_state = server_state::normal;

    write(m_bs, status{ status::OK });
    write(m_bs, free_space::response{ .space_available = space });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_reset()
{
    DEBUG << "reset request on " << client_->peer();

    reset::request req;
    read(m_bs, req);

    m_handler_interface->on_reset();

    m_state = server_state::normal;
    m_read_block.reset();
    m_write_alloc.reset();

    write(m_bs, status{ status::OK });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_next_chunk()
{
    DEBUG << "next_chunk request on " << client_->peer();

    next_chunk::request req;
    read(m_bs, req);

    if (req.max_size < MINIMUM_CHUNK_SIZE || req.max_size > MAXIMUM_CHUNK_SIZE)
    {
        THROW(util::illegal_args, "buffer size out of range");
    }

    std::vector<char> buffer(req.max_size);
    auto count = m_read_block->read(std::span<char>(buffer.begin(), buffer.end()));
    if (count == 0)
    {
        m_state = server_state::normal;
        m_read_block.reset();
    }

    m_handler_interface->on_next_chunk(buffer);
    write(m_bs, status{ status::OK });
    write(m_bs, next_chunk::response{ .content = std::span<char>(buffer.begin(), count) });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_allocate_chunk()
{
    DEBUG << "allocate request on " << client_->peer();

    allocate_chunk::request req;
    read(m_bs, req);

    if (req.size > MAXIMUM_BLOCK_SIZE)
    {
        THROW(util::illegal_args, "block size out of range");
    }

    m_write_alloc = m_handler_interface->on_allocate_chunk(req.size);
    m_state = server_state::writing;

    write(m_bs, status{ status::OK });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_write_small_block ()
{
    DEBUG << "write_short_block request on " << client_->peer();

    std::vector<char> buffer(SMALL_CHUNK_LIMIT);
    write_small_block::request req{ .data = buffer };
    read(m_bs, req);
    auto meta_data = m_handler_interface->on_write_small_block (req.data);
    write(m_bs, status{ status::OK });
    write(m_bs, write_small_block::response{
            .hash = std::move(meta_data.hash),
            .effective_size = meta_data.effective_size });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_read_small_block ()
{
    DEBUG << "read_short_block request on " << client_->peer();
    THROW(util::exception, "read_short_block not implemented");
}

// ---------------------------------------------------------------------

void server::handle_write_chunk()
{
    DEBUG << "write_chunk request on " << client_->peer();

    std::vector<char> buffer(MAXIMUM_CHUNK_SIZE);
    write_chunk::request req{ .data = buffer };
    read(m_bs, req);

    if (!m_write_alloc)
    {
        THROW(internal_error, "no space allocated");
    }

    m_handler_interface->on_write_chunk(req.data);
    m_write_alloc->device().write(req.data);
    write(m_bs, status{ status::OK });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_finalize_block()
{
    DEBUG << "finalize_block request on " << client_->peer();

    finalize_block::request req;
    read(m_bs, req);

    if (!m_write_alloc)
    {
        THROW(internal_error, "no space allocated");
    }

    m_handler_interface->on_finalize();

    auto meta_data = m_write_alloc->persist();
    m_write_alloc.reset();

    write(m_bs, status{ status::OK });
    write(m_bs, finalize_block::response{
            .hash = std::move(meta_data.hash),
            .effective_size = meta_data.effective_size });
    m_bs.sync ();

    m_state = server_state::normal;
}

// ---------------------------------------------------------------------

void server::handle_client_statistics()
{
    DEBUG << "client_statistics request on " << client_->peer();

    client_statistics::request req;
    read(m_bs, req);

    DEBUG << "{ " << (req.uhv_id[0]) << ", " << req.integrated_size << " }";

    write(m_bs, status{ status::OK });
    m_bs.sync();

}

// ---------------------------------------------------------------------

} // namespace uh::protocol
