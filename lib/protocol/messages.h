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

struct write_block
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

void write(std::ostream& out, const write_block::request& request);
void read(std::istream& in, write_block::request& request);

void read(std::istream& in, write_block::response& response);
void write(std::ostream& out, const write_block::response& response);

// ---------------------------------------------------------------------

struct read_block
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

} // namespace uh::protocol

#endif
