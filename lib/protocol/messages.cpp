#include "messages.h"

#include "serializer.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

void write(std::ostream& out, const protocol::status& status)
{
    write(out, status.code);

    if (status.code != status::OK)
    {
        write(out, status.message);
    }
}

// ---------------------------------------------------------------------

void check_status(std::istream& in)
{
    uint8_t code;
    read(in, code);

    if (static_cast<status::code_t>(code) != status::OK)
    {
        std::string message;
        read(in, message);
        throw std::runtime_error(message);
    }
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const hello::request& request)
{
    write(out, hello::request_id);
    write(out, request.client_version);
}

// ---------------------------------------------------------------------

void read(std::istream& in, hello::request& request)
{
    hello::request tmp;

    read(in, tmp.client_version);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const hello::response& response)
{
    write(out, response.server_version);
    write(out, response.protocol_version);
}

// ---------------------------------------------------------------------

void read(std::istream& in, hello::response& request)
{
    check_status(in);

    hello::response tmp;

    read(in, tmp.server_version);
    read(in, tmp.protocol_version);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const write_block::request& request)
{
    write(out, write_block::request_id);
    write(out, request.content);
}

// ---------------------------------------------------------------------

void read(std::istream& in, write_block::request& request)
{
    write_block::request tmp;
    read(in, tmp.content);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const write_block::response& response)
{
    write(out, response.hash);
    write(out, response.effective_size);
}

// ---------------------------------------------------------------------

void read(std::istream& in, write_block::response& response)
{
    check_status(in);

    write_block::response tmp;
    read(in, tmp.hash);
    read(in, tmp.effective_size);

    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const read_block::request& request)
{
    write(out, read_block::request_id);
    write(out, request.hash);
}

// ---------------------------------------------------------------------

void read(std::istream& in, read_block::request& request)
{
    read_block::request tmp;

    read(in, tmp.hash);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const read_block::response& response)
{
}

// ---------------------------------------------------------------------

void read(std::istream& in, read_block::response& response)
{
    check_status(in);

    read_block::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const quit::request& request)
{
    write(out, quit::request_id);
    write(out, request.reason);
}

// ---------------------------------------------------------------------

void read(std::istream& in, quit::request& request)
{
    quit::request tmp;

    read(in, tmp.reason);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream&, const quit::response&)
{
}

// ---------------------------------------------------------------------

void read(std::istream& in, quit::response&)
{
    check_status(in);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const free_space::request& request)
{
    write(out, free_space::request_id);
}

// ---------------------------------------------------------------------

void read(std::istream& in, free_space::request& request)
{
    free_space::request tmp;
    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const free_space::response& response)
{
    write(out, response.space_available);
}

// ---------------------------------------------------------------------

void read(std::istream& in, free_space::response& response)
{
    check_status(in);

    free_space::response tmp;

    read(in, tmp.space_available);

    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const reset::request& request)
{
    write(out, reset::request_id);
}

// ---------------------------------------------------------------------

void read(std::istream& in, reset::request& request)
{
    reset::request tmp;
    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const reset::response&)
{
}

// ---------------------------------------------------------------------

void read(std::istream& in, reset::response& response)
{
    check_status(in);

    reset::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const next_chunk::request& request)
{
    write(out, next_chunk::request_id);
    write(out, request.max_size);
}

// ---------------------------------------------------------------------

void read(std::istream& in, next_chunk::request& request)
{
    next_chunk::request tmp;

    read(in, tmp.max_size);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const next_chunk::response& response)
{
    write(out, response.content);
}

// ---------------------------------------------------------------------

void read(std::istream& in, next_chunk::response& response)
{
    check_status(in);
    read(in, response.content);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const allocate_chunk::request& request)
{
    write(out, allocate_chunk::request_id);
    write(out, request.size);
}

// ---------------------------------------------------------------------

void read(std::istream& in, allocate_chunk::request& request)
{
    allocate_chunk::request tmp;

    read(in, tmp.size);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const allocate_chunk::response&)
{
}

// ---------------------------------------------------------------------

void read(std::istream& in, allocate_chunk::response& response)
{
    check_status(in);

    allocate_chunk::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const write_chunk::request& request)
{
    write(out, write_chunk::request_id);
    write(out, request.data);
}

// ---------------------------------------------------------------------

void read(std::istream& in, write_chunk::request& request)
{
    write_chunk::request tmp;

    read(in, tmp.data);

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const write_chunk::response& response)
{
}

// ---------------------------------------------------------------------

void read(std::istream& in, write_chunk::response& response)
{
    check_status(in);

    write_chunk::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
