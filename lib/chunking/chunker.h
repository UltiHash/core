#ifndef CHUNKING_CHUNKER_H
#define CHUNKING_CHUNKER_H

#include <cstdint>
#include <span>
#include <vector>


namespace uh::chunking
{

// ---------------------------------------------------------------------

class chunker
{
public:
    virtual ~chunker() = default;

    /**
     * Compute the chunk sizes for a given buffer.
     */
    virtual std::vector<uint32_t> chunk(std::span<char> b) const = 0;

    /**
     * Return minimum chunk size.
     */
    virtual std::size_t min_size() const = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
