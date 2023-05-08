#ifndef CHUNKING_CHUNKER_H
#define CHUNKING_CHUNKER_H

#include <span>
#include "buffer.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

class chunker
{
public:
    virtual ~chunker() = default;

    /**
     * Return the next chunk to upload. If there are no more chunks, return
     * an empty chunk instead.
     *
     * @throw may throw any derivative of exception on error
     */
    virtual std::span<char> next_chunk() = 0;
    [[nodiscard]] virtual buffer& get_buffer () = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
