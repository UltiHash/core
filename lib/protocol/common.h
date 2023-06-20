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
    uint64_t effective_size;
};

// ---------------------------------------------------------------------

struct chunks_meta_data
{
    std::vector <char> data;
    std::vector <std::uint32_t> chunk_sizes;
    std::list <uint32_t> chunk_indices;
};

template <typename T>
struct owning_span {
    std::size_t size {};
    std::unique_ptr <T[]> data = nullptr;
    owning_span() = default;
    owning_span(size_t data_size):
        size (data_size),
        data {std::make_unique_for_overwrite <T[]> (size)} {}
    owning_span(size_t data_size, std::unique_ptr <T[]>&& ptr):
        size (data_size),
        data {std::move (ptr)} {}

};

template <typename T>
using ospan = owning_span <T>;

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
