#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

#include <string>
#include <vector>


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

} // namespace uh::protocol

#endif
