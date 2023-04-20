#ifndef CHUNKING_CHUNKER_H
#define CHUNKING_CHUNKER_H

#include <span>


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
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
