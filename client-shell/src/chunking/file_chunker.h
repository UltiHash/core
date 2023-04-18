#ifndef CLIENT_FILE_CHUNKING_H
#define CLIENT_FILE_CHUNKING_H

#include <span>


namespace uh::client::chunking {

// ---------------------------------------------------------------------

class file_chunker {
public:
    virtual ~file_chunker() = default;

    /**
     * Return the next chunk to upload. If there are no more chunks, return
     * an empty chunk instead.
     *
     * @throw may throw any derivative of exception on error
     */
    virtual std::span<char> next_chunk() = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
