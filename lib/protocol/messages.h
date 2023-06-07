#ifndef PROTOCOL_MESSAGES_H
#define PROTOCOL_MESSAGES_H

#include "common.h"

#include <serialization/serialization.h>

#include <span>
#include <string>
#include <variant>
#include <vector>


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
        std::variant<std::span <char>, std::span<const char>> hashes;
    };

    struct response
    {
        std::variant< std::vector<char>, std::unique_ptr<io::data_generator> > data;
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
