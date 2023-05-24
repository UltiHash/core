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
        case quit::request_id: return handle_quit();
        case free_space::request_id: return handle_free_space();
        case reset::request_id: return handle_reset();
        case client_statistics::request_id: return handle_client_statistics();
        case write_chunks::request_id: return handle_write_chunks();
        case read_chunks::request_id: return handle_read_chunks();

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
    m_write_alloc.reset();

    write(m_bs, status{ status::OK });
    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_client_statistics()
{
    DEBUG << "client_statistics request on " << client_->peer();

    client_statistics::request req;
    read(m_bs, req);

    m_handler_interface->on_client_statistics(req);

    write(m_bs, status{ status::OK });
    m_bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_write_chunks() {
    DEBUG << "write_chunks request on " << client_->peer();

    auto chunk_sizes = m_bs.read<std::vector <uint32_t>>();
    auto data = m_bs.read<std::vector <char>>();
    auto resp = m_handler_interface->on_write_chunks ({chunk_sizes, data});

    write(m_bs, status{ status::OK });
    write(m_bs, resp);

    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_read_chunks() {
    DEBUG << "read_chunks request on " << client_->peer();

    auto hashes = m_bs.read<std::vector <char>>();
    auto resp = m_handler_interface->on_read_chunks ({{hashes.data(), hashes.size()}});

    write(m_bs, status{ status::OK });
    write(m_bs, resp);

    m_bs.sync ();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
