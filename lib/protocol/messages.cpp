#include "messages.h"

#include "serializer.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const protocol::status& status)
{
    out.write(status.code);
    out.write(status.code);

    if (status.code != status::OK)
    {
        out.write(status.message);
    }
}

// ---------------------------------------------------------------------

void check_status(serialization::buffered_serialization& in)
{
    const auto code = in.read <uint8_t> ();

    if (static_cast<status::code_t>(code) != status::OK)
    {
        const auto message = in.read<std::string> ();
        throw std::runtime_error(message);
    }
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const hello::request& request)
{
    out.write(hello::request_id);
    out.write(request.client_version);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, hello::request& request)
{
    hello::request tmp {in.read<std::string>()};

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const hello::response& response)
{
    out.write(response.server_version);
    out.write(response.protocol_version);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, hello::response& request)
{
    check_status(in);

    hello::response tmp;

    tmp.server_version = in.read<std::string> ();
    tmp.server_version = in.read<unsigned> ();

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_block::request& request)
{
    out.write(write_block::request_id);
    out.write(request.content);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_block::request& request)
{
    write_block::request tmp;
    tmp.content = in.read<std::vector<char>> ();
    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_block::response& response)
{
    out.write(response.hash);
    out.write(response.effective_size);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_block::response& response)
{
    check_status(in);

    write_block::response tmp;
    tmp.hash = in.read<std::vector <char>>();
    tmp.effective_size = in.read<uint64_t>();

    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_block::request& request)
{
    out.write(read_block::request_id);
    out.write(request.hash);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, read_block::request& request)
{
    read_block::request tmp;

    tmp.hash = in.read<std::vector <char>> ();

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_block::response& response)
{
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, read_block::response& response)
{
    check_status(in);

    read_block::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const quit::request& request)
{
    out.write(quit::request_id);
    out.write(request.reason);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, quit::request& request)
{
    quit::request tmp;

    tmp.reason = in.read <std::string> ();

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(std::ostream&, const quit::response&)
{
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, quit::response&)
{
    check_status(in);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const free_space::request& request)
{
    out.write(free_space::request_id);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, free_space::request& request)
{
    free_space::request tmp;
    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const free_space::response& response)
{
    out.write(response.space_available);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, free_space::response& response)
{
    check_status(in);

    free_space::response tmp {};

    tmp.space_available = in.read <uint64_t> ();

    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const reset::request& request)
{
    out.write(reset::request_id);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, reset::request& request)
{
    reset::request tmp;
    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const reset::response&)
{
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, reset::response& response)
{
    check_status(in);

    reset::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const next_chunk::request& request)
{
    out.write(next_chunk::request_id);
    out.write(request.max_size);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, next_chunk::request& request)
{
    next_chunk::request tmp {};

    tmp.max_size = in.read <uint32_t> ();

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const next_chunk::response& response)
{
    out.write(response.content);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, next_chunk::response& response)
{
    check_status(in);

    const auto tmp = in.read<std::vector <char> >();
    std::memcpy(response.content.data(), tmp.data(), tmp.size());
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
