#ifndef CLIENT_CHUNKING_FIXED_SIZE_H
#define CLIENT_CHUNKING_FIXED_SIZE_H

#include "io/device.h"
#include "chunker.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

class fixed_size_chunker : public uh::chunking::chunker
{
public:
    fixed_size_chunker(io::device& dev, size_t chunk_size, std::size_t buffer_size = 0);

    /**
     * Return the next chunk to upload. If there are no more chunks, return
     * an empty chunk instead.
     *
     * @throw may throw any derivative of exception on error
     */
    std::span<char> next_chunk() override;
    [[nodiscard]] uh::chunking::buffer& get_buffer () override;

private:
    size_t m_chunk_size = 0;
    uh::chunking::buffer m_buffer;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
