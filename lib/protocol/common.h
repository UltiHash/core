#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <span>

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
    uint64_t effective_size = 0u;
};

// ---------------------------------------------------------------------

struct chunks_meta_data
{
    std::vector <char> data;
    std::vector <std::uint32_t> chunk_sizes;
    std::list <uint32_t> chunk_indices;
};



// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
