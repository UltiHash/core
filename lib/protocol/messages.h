#ifndef PROTOCOL_MESSAGES_H
#define PROTOCOL_MESSAGES_H

#include "common.h"

#include <span>
#include <string>
#include "serialization/serialization.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

struct status
{
    enum code_t : uint8_t
    {
        OK = 0,
        FAILED = 1
    };

    code_t code;
    std::string message;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const uh::protocol::status& status);
void check_status(serialization::buffered_serialization& in);

// ---------------------------------------------------------------------

struct hello
{
    struct request
    {
        std::string client_version;
    };

    struct response
    {
        std::string server_version;
        unsigned protocol_version;
    };

    constexpr static uint8_t request_id = 0x01;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const hello::request& request);
void read(serialization::buffered_serialization& out, hello::request& request);

void write(serialization::buffered_serialization& out, const hello::response& response);
void read(serialization::buffered_serialization& in, hello::response& request);

// ---------------------------------------------------------------------

struct read_block
{
    struct request
    {
        blob hash;
    };

    struct response
    {
    };

    constexpr static uint8_t request_id = 0x03;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_block::request& request);
void read(serialization::buffered_serialization& in, read_block::request& request);

void write(serialization::buffered_serialization& out, const read_block::response& response);
void read(serialization::buffered_serialization& in, read_block::response& response);

// ---------------------------------------------------------------------

struct quit
{
    struct request
    {
        std::string reason;
    };

    struct response
    {
    };

    constexpr static uint8_t request_id = 0x04;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const quit::request& request);
void read(serialization::buffered_serialization& in, quit::request& request);

void write(serialization::buffered_serialization& out, const quit::response& response);
void read(serialization::buffered_serialization& in, quit::response& response);

// ---------------------------------------------------------------------

struct free_space
{
    struct request
    {
    };

    struct response
    {
        uint64_t space_available;
    };

    constexpr static uint8_t request_id = 0x05;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const free_space::request& request);
void read(serialization::buffered_serialization& in, free_space::request& request);

void write(serialization::buffered_serialization& out, const free_space::response& response);
void read(serialization::buffered_serialization& in, free_space::response& response);

// ---------------------------------------------------------------------

struct reset
{
    struct request
    {
    };

    struct response
    {
    };

    constexpr static uint8_t request_id = 0x06;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const reset::request& request);
void read(serialization::buffered_serialization& in, reset::request& request);

void write(serialization::buffered_serialization& out, const reset::response& response);
void read(serialization::buffered_serialization& in, reset::response& response);

// ---------------------------------------------------------------------

struct next_chunk
{
    struct request
    {
        uint32_t max_size;
    };

    struct response
    {
        std::span<char> content;
    };

    constexpr static uint8_t request_id = 0x07;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const next_chunk::request& request);
void read(serialization::buffered_serialization& in, next_chunk::request& request);

void write(serialization::buffered_serialization& out, const next_chunk::response& response);
void read(serialization::buffered_serialization& in, next_chunk::response& response);

// ---------------------------------------------------------------------

struct allocate_chunk
{
    struct request
    {
        uint64_t size;
    };

    struct response
    {
    };

    constexpr static uint8_t request_id = 0x08;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const allocate_chunk::request& request);
void read(serialization::buffered_serialization& in, allocate_chunk::request& request);

void write(serialization::buffered_serialization& out, const allocate_chunk::response& response);
void read(serialization::buffered_serialization& in, allocate_chunk::response& response);

// ---------------------------------------------------------------------

struct write_chunk
{
    struct request
    {
        std::span<char> data;
    };

    struct response
    {
    };

    constexpr static uint8_t request_id = 0x09;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_chunk::request& request);
void read(serialization::buffered_serialization& in, write_chunk::request& request);

void write(serialization::buffered_serialization& out, const write_chunk::response& response);
void read(serialization::buffered_serialization& in, write_chunk::response& response);

// ---------------------------------------------------------------------

struct finalize_block
{
    struct request
    {
    };

    struct response
    {
        blob hash;
        uint64_t effective_size;
    };

    constexpr static uint8_t request_id = 0x0a;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const finalize_block::request& request);
void read(serialization::buffered_serialization& in, finalize_block::request& request);

void write(serialization::buffered_serialization& out, const finalize_block::response& response);
void read(serialization::buffered_serialization& in, finalize_block::response& response);

// ---------------------------------------------------------------------

struct write_small_block
{
    struct request
    {
        std::span<char> data;
    };

    struct response
    {
        blob hash;
        uint64_t effective_size;
    };

    constexpr static uint8_t request_id = 0x0b;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_small_block::request& request);
void read(serialization::buffered_serialization& in, write_small_block::request& request);

void write(serialization::buffered_serialization& out, const write_small_block::response& response);
void read(serialization::buffered_serialization& in, write_small_block::response& response);

// ---------------------------------------------------------------------

struct read_small_block
{
    struct request
    {
        blob hash;
    };

    struct response
    {
        std::span<char> data;
    };

    constexpr static uint8_t request_id = 0x0c;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_small_block::request& request);
void read(serialization::buffered_serialization& in, read_small_block::request& request);

void write(serialization::buffered_serialization& out, const read_small_block::response& response);
void read(serialization::buffered_serialization& in, read_small_block::response& response);

// ---------------------------------------------------------------------

struct read_xsmall_blocks
{
    struct request
    {
        blob hashes;
    };

    struct response
    {
        std::vector <char> data;
    };

    constexpr static uint8_t request_id = 0x0e;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_xsmall_blocks::request& request);
void read(serialization::buffered_serialization& in, read_xsmall_blocks::request& request);

void write(serialization::buffered_serialization& out, const read_xsmall_blocks::response& response);
void read(serialization::buffered_serialization& in, read_xsmall_blocks::response& response);

// ---------------------------------------------------------------------


struct client_statistics
{
    struct request
    {
        blob uhv_id;
        std::uint64_t integrated_size;
    };

    struct response
    {
    };
    constexpr static uint8_t request_id = 0x0f;
};

void write(serialization::buffered_serialization& out, const client_statistics::request& request);
void read(serialization::buffered_serialization& in, client_statistics::request& request);

void write(serialization::buffered_serialization& out, const client_statistics::response& response);
void read(serialization::buffered_serialization& in, client_statistics::response& response);

// ---------------------------------------------------------------------


struct write_chunks
{
    struct request
    {
        std::span <uint32_t> chunk_sizes;
        std::span <char> data;
    };

    struct response
    {
        std::vector <char> hashes;
        std::size_t effective_size;
    };

    constexpr static uint8_t request_id = 0x10;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const write_chunks::request& request);
void read(serialization::buffered_serialization& in, write_chunks::request& request);

void write(serialization::buffered_serialization& out, const write_chunks::response& response);
void read(serialization::buffered_serialization& in, write_chunks::response& response);

// ---------------------------------------------------------------------


struct read_chunks
{
    struct request
    {
        std::span <char> hashes;
    };

    struct response
    {
        std::vector <char> data;
        std::vector <uint32_t> chunk_sizes;
    };

    constexpr static uint8_t request_id = 0x11;
};

// ---------------------------------------------------------------------

void write(serialization::buffered_serialization& out, const read_chunks::request& request);
void read(serialization::buffered_serialization& in, read_chunks::request& request);

void write(serialization::buffered_serialization& out, const read_chunks::response& response);
void read(serialization::buffered_serialization& in, read_chunks::response& response);

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
