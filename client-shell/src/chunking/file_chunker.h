#ifndef CLIENT_FILE_CHUNKING_H
#define CLIENT_FILE_CHUNKING_H

#include <protocol/common.h>


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
    virtual uh::protocol::blob next_chunk() = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
