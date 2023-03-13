#include "server.h"


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

void server::handle()
{


    m_state = server_state::setup;

    while (client_->valid() && m_state != server_state::disconnected)
    {
        try
        {
            const auto request_id = bs.read <uint8_t> ();

            switch (m_state)
            {
                case server_state::setup: handle_setup_request(request_id); break;
                case server_state::normal: handle_normal_request(request_id); break;
                case server_state::reading: handle_reading_request(request_id); break;

                default:
                    write(bs, status{ .code = status::FAILED, .message = "unsupported state" });
                    bs.sync ();
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
            write(bs, status{ .code = status::FAILED, .message = e.what() });
            bs.sync ();

            m_block.reset();
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
        case write_block::request_id: return handle_write_block();
        case read_block::request_id: return handle_read_block();
        case quit::request_id: return handle_quit();
        case free_space::request_id: return handle_free_space();
        case reset::request_id: return handle_reset();

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

void server::handle_hello()
{
    hello::request req;
    read(bs, req);

    server_information info;

    try
    {
        info = on_hello(req.client_version);
    }
    catch (const std::exception& e)
    {
        write(bs, status{ .code = status::FAILED, .message = e.what() });
        m_state = server_state::disconnected;
        return;
    }

    write(bs, status{ status::OK });
    write(bs, hello::response{
        .server_version = info.version,
        .protocol_version = info.protocol });
    bs.sync();

    m_state = server_state::normal;
}

// ---------------------------------------------------------------------

void server::handle_write_block()
{
    write_block::request req;
    read(bs, req);

    block_meta_data blockMetaData = on_write_block(std::move(req.content));

    m_state = server_state::normal;

    write(bs, status{ status::OK });
    write(bs, write_block::response{ std::move(blockMetaData.hash), blockMetaData.effective_size });
    bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_read_block()
{
    read_block::request req;
    read(bs, req);

    m_block = on_read_block(std::move(req.hash));

    m_state = server_state::reading;

    write(bs, status{ status::OK });
    bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_quit()
{
    quit::request req;
    read(bs, req);

    try
    {
        on_quit(req.reason);
    }
    catch (...)
    {
        // ignore
    }

    m_state = server_state::disconnected;

    write(bs, status{ status::OK });
    bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_free_space()
{
    free_space::request req;
    read(bs, req);

    auto space = on_free_space();

    m_state = server_state::normal;

    write(bs, status{ status::OK });
    write(bs, free_space::response{ .space_available = space });
    bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_reset()
{
    reset::request req;
    read(bs, req);

    on_reset();

    m_state = server_state::normal;
    m_block.reset();

    write(bs, status{ status::OK });
    bs.sync();
}

// ---------------------------------------------------------------------

void server::handle_next_chunk()
{
    next_chunk::request req;
    read(bs, req);

    if (req.max_size < MINIMUM_CHUNK_SIZE || req.max_size > MAXIMUM_CHUNK_SIZE)
    {
        THROW(illegal_args, "buffer size out of range");
    }

    std::vector<char> buffer(req.max_size);
    auto count = m_block->read(std::span<char>(buffer.begin(), buffer.end()));
    if (count == 0)
    {
        m_state = server_state::normal;
        m_block.reset();
    }

    write(bs, status{ status::OK });
    write(bs, next_chunk::response{ .content = std::span<char>(buffer.begin(), count) });
    bs.sync();
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
