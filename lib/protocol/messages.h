#ifndef PROTOCOL_MESSAGES_H
#define PROTOCOL_MESSAGES_H

#include "common.h"

#include <span>
#include <string>


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

void write(std::ostream& out, const uh::protocol::status& status);
void check_status(std::istream& in);

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

void write(std::ostream& out, const hello::request& request);
void read(std::istream& out, hello::request& request);

void write(std::ostream& out, const hello::response& response);
void read(std::istream& in, hello::response& request);

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

void write(std::ostream& out, const read_block::request& request);
void read(std::istream& in, read_block::request& request);

void write(std::ostream& out, const read_block::response& response);
void read(std::istream& in, read_block::response& response);

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

void write(std::ostream& out, const quit::request& request);
void read(std::istream& in, quit::request& request);

void write(std::ostream& out, const quit::response& response);
void read(std::istream& in, quit::response& response);

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

void write(std::ostream& out, const free_space::request& request);
void read(std::istream& in, free_space::request& request);

void write(std::ostream& out, const free_space::response& response);
void read(std::istream& in, free_space::response& response);

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

void write(std::ostream& out, const reset::request& request);
void read(std::istream& in, reset::request& request);

void write(std::ostream& out, const reset::response& response);
void read(std::istream& in, reset::response& response);

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

void write(std::ostream& out, const next_chunk::request& request);
void read(std::istream& in, next_chunk::request& request);

void write(std::ostream& out, const next_chunk::response& response);
void read(std::istream& in, next_chunk::response& response);

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

void write(std::ostream& out, const allocate_chunk::request& request);
void read(std::istream& in, allocate_chunk::request& request);

void write(std::ostream& out, const allocate_chunk::response& response);
void read(std::istream& in, allocate_chunk::response& response);

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

void write(std::ostream& out, const write_chunk::request& request);
void read(std::istream& in, write_chunk::request& request);

void write(std::ostream& out, const write_chunk::response& response);
void read(std::istream& in, write_chunk::response& response);

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

void write(std::ostream& out, const finalize_block::request& request);
void read(std::istream& in, finalize_block::request& request);

void write(std::ostream& out, const finalize_block::response& response);
void read(std::istream& in, finalize_block::response& response);

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
