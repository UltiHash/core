#include "messages.h"

namespace uh::protocol
{

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const protocol::status& status)
{
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
    tmp.protocol_version = in.read<unsigned> ();

    std::swap(tmp, request);
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
    tmp.hash = in.read<std::vector<char>> ();
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

void write(serialization::buffered_serialization&, const quit::response&)
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
    in.read(response.content);
}

void write(serialization::buffered_serialization& out, const allocate_chunk::request& request)
{
    out.write(allocate_chunk::request_id);
    out.write(request.size);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, allocate_chunk::request& request)
{
    allocate_chunk::request tmp;
    tmp.size = in.read <uint64_t>();

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const allocate_chunk::response&)
{
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, allocate_chunk::response& response)
{
    check_status(in);

    allocate_chunk::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_chunk::request& request)
{
    out.write(write_chunk::request_id);
    out.write(request.data);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_chunk::request& request)
{
    in.read(request.data);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_chunk::response& response)
{
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_chunk::response& response)
{
    check_status(in);

    write_chunk::response tmp;
    std::swap(tmp, response);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const finalize_block::request& request)
{
    out.write(finalize_block::request_id);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, finalize_block::request& request)
{
    finalize_block::request tmp;
    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const finalize_block::response& response)
{
    out.write(response.hash);
    out.write(response.effective_size);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, finalize_block::response& response)
{
    check_status(in);

    response.hash = in.read<std::vector<char>>();
    response.effective_size = in.read<uint64_t>();
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_small_block::request& request)
{
    out.write(write_small_block::request_id);
    out.write(request.data);}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_small_block::request& request)
{
    in.read(request.data);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_small_block::response& response)
{
    out.write(response.hash);
    out.write(response.effective_size);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_small_block::response& response)
{
    check_status(in);

    response.hash = in.read<std::vector<char>>();
    response.effective_size = in.read<decltype (response.effective_size)>();
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_small_block::request& request)
{
    out.write(read_small_block::request_id);
    out.write(request.hash);}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, read_small_block::request& request)
{
    request.hash = in.read <std::vector<char>> ();
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_small_block::response& response)
{
    out.write(response.data);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, read_small_block::response& response)
{
    check_status(in);
    in.read (response.data);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const client_statistics::request& request)
{
    out.write(client_statistics::request_id);
    out.write(request.uhv_id);
    out.write(request.integrated_size);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, client_statistics::request& request)
{
    client_statistics::request tmp;

    tmp.uhv_id = in.read<blob>();
    tmp.integrated_size = in.read<std::uint64_t>();

    std::swap(tmp, request);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const client_statistics::response& response)
{
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, client_statistics::response& response)
{
    check_status(in);

    client_statistics::response tmp;
    std::swap(tmp, response);
}
// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_chunks::request& request)
{
    out.write(write_small_block::request_id);
    out.write(request.data);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_chunks::request& request)
{
    in.read(request.data);
}

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_chunks::response& response)
{
    out.write(response.hashes);
    out.write(response.effective_sizes);
}

// ---------------------------------------------------------------------

void read(serialization::buffered_serialization& in, write_chunks::response& response)
{
    check_status(in);

    response.hashes = in.read<std::vector<char>>();
    response.effective_sizes = in.read<decltype (response.effective_sizes)>();
}
// ---------------------------------------------------------------------

} // namespace uh::protocol
