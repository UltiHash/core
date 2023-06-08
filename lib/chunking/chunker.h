#ifndef CHUNKING_CHUNKER_H
#define CHUNKING_CHUNKER_H

#include <span>


namespace uh::chunking
{

// ---------------------------------------------------------------------

struct chunk_result
{
    enum result_type
    {
        // a chunk of size `size` was created at the provided buffer address
        created,
        // there are no more chunks available
        done,
        // the provided buffer is too small
        too_small
    };

    result_type type;
    std::size_t size = 0;
};

// ---------------------------------------------------------------------

class chunker
{
public:
    virtual ~chunker() = default;

    /**
     * Create a new chunk in the provided buffer.
     *
     * The chunker returns `chunk_result::done` if no more chunks will be
     * created.
     *
     * The chunker returns `chunk_result::created` with an additional `size`
     * to signal that a chunk was created at the start of `b`. The chunker
     * expects the caller to pass `b.data() + size` next time and may already
     * fill the provided buffer to reduce I/O calls.
     *
     * The chunker returns `chunk_result::too_small` to signal that the provided
     * buffer is insufficient to hold a full chunk and requests to pass a
     * sufficiently large buffer next time. The chunker will not use the provided
     * buffer to store data.
     */
    virtual chunk_result chunk(std::span<char> b) = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
