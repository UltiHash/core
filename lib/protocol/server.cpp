#include "server.h"

#include <protocol/exception.h>
#include <protocol/messages.h>

#include <logging/logging_boost.h>


using namespace boost::asio;

namespace uh::protocol
{

// ---------------------------------------------------------------------

server::server(const std::shared_ptr<net::socket>& client,
               std::unique_ptr<request_interface>&& handler_interface)
    : m_client(client),
      m_bs(*m_client),
      m_handler_interface(std::move(handler_interface))
{
}

// ---------------------------------------------------------------------

void server::handle()
{
    try
    {
        switch (m_bs.read<uint8_t>())
        {
            case hello::request_id:
                handle_hello();
                break;
            case quit::request_id:
                handle_quit();
                return;

            default:
                throw std::runtime_error("unsupported command");
        }
    }
    catch (const std::exception& e)
    {
        write(m_bs, status{ .code = status::FAILED, .message = e.what() });
        m_bs.sync();
        return;
    }

    while (m_client->valid())
    {
        try
        {
            handle_normal_request(m_bs.read<uint8_t>());
        }
        catch (const read_error& e)
        {
            ERROR << e.what();
            break;
        }
        catch (const std::exception& e)
        {
            write(m_bs, status{ .code = status::FAILED, .message = e.what() });
            m_bs.sync();
        }
    }
}

// ---------------------------------------------------------------------

void server::handle_normal_request(uint8_t request_id)
{
    switch (request_id)
    {
        case quit::request_id: return handle_quit();
        case free_space::request_id: return handle_free_space();
        case client_statistics::request_id: return handle_client_statistics();
        case write_chunks::request_id: return handle_write_chunks();
        case read_chunks::request_id: return handle_read_chunks();

        default:
            throw std::runtime_error("normal, unsupported command: "
                                     + std::to_string(request_id));
    }
}

// ---------------------------------------------------------------------

void server::handle_hello()
{
    DEBUG << "hello request on " << m_client->peer();
    hello::request req;
    read(m_bs, req);

    server_information info = m_handler_interface->on_hello(req.client_version);

    write(m_bs, status{ status::OK });
    write(m_bs, hello::response{
            .server_version = info.version,
            .protocol_version = info.protocol });

    m_bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_quit()
{
    DEBUG << "quit request on " << m_client->peer();

    quit::request req;
    read(m_bs, req);

    m_handler_interface->on_quit(req.reason);

    write(m_bs, status{ status::OK });

    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_free_space()
{
    DEBUG << "free_space request on " << m_client->peer();

    free_space::request req;
    read(m_bs, req);

    auto space = m_handler_interface->on_free_space();

    write(m_bs, status{ status::OK });
    write(m_bs, free_space::response{ .space_available = space });

    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_client_statistics()
{
    DEBUG << "client_statistics request on " << m_client->peer();

    client_statistics::request req;
    read(m_bs, req);

    m_handler_interface->on_client_statistics(req);

    write(m_bs, status{ status::OK });

    m_bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_write_chunks()
{
    DEBUG << "write_chunks request on " << m_client->peer();

    auto chunk_sizes = m_bs.read<std::vector <uint32_t>>();
    auto data = m_bs.read<std::vector <char>>();
    auto resp = m_handler_interface->on_write_chunks ({chunk_sizes, data});

    write(m_bs, status{ status::OK });
    write(m_bs, resp);

    m_bs.sync ();
}

// ---------------------------------------------------------------------

void server::handle_read_chunks()
{
    DEBUG << "read_chunks request on " << m_client->peer();

    auto hashes = m_bs.read<std::vector <char>>();
    auto resp = m_handler_interface->on_read_chunks ({{hashes.data(), hashes.size()}});

    write(m_bs, status{ status::OK });
    write(m_bs, resp);

    m_bs.sync ();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
