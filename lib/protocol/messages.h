#ifndef PROTOCOL_MESSAGES_H
#define PROTOCOL_MESSAGES_H

#include "common.h"

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

struct write_chunk
{
    struct request
    {
        blob content;
    };

    struct response
    {
        blob hash;
    };

    constexpr static uint8_t request_id = 0x02;
};

// ---------------------------------------------------------------------

void write(std::ostream& out, const write_chunk::request& request);
void read(std::istream& in, write_chunk::request& request);

void read(std::istream& in, write_chunk::response& response);
void write(std::ostream& out, const write_chunk::response& response);

// ---------------------------------------------------------------------

struct read_chunk
{
    struct request
    {
        blob hash;
    };

    struct response
    {
        blob content;
    };

    constexpr static uint8_t request_id = 0x03;
};

// ---------------------------------------------------------------------

void write(std::ostream& out, const read_chunk::request& request);
void read(std::istream& in, read_chunk::request& request);

void write(std::ostream& out, const read_chunk::response& response);
void read(std::istream& in, read_chunk::response& response);

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
