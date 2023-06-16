#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

#include <string>
#include <vector>
#include <list>


namespace uh::protocol
{

// ---------------------------------------------------------------------

typedef std::vector<char> blob;

// ---------------------------------------------------------------------

struct server_information
{
    std::string version;
    unsigned protocol;
};

// ---------------------------------------------------------------------

struct block_meta_data
{
    blob hash;
    uint64_t effective_size;
};

// ---------------------------------------------------------------------

struct chunks_meta_data
{
    std::vector <char> data;
    std::vector <std::uint32_t> chunk_sizes;
    std::list <uint32_t> chunk_indices;
};

struct key_value_t {
    std::span <const char> key;
    std::span <const char> value;
};

struct data_entry {
    key_value_t key_value;
    std::span <std::span <const char>> labels;
};

struct query {
    std::span <char> single_key;
    std::span <char> start_key;
    std::span <char> end_key;
    std::span <std::span <const char>> labels;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
