#ifndef CLIENT_CHUNKING_FIXED_SIZE_H
#define CLIENT_CHUNKING_FIXED_SIZE_H

#include <chunking/file_chunker.h>
#include <io/device.h>


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

class fixed_size_chunker : public file_chunker
{
public:
    fixed_size_chunker(io::device& dev, size_t chunk_size);

    /**
     * Return the next chunk to upload. If there are no more chunks, return
     * an empty chunk instead.
     *
     * @throw may throw any derivative of exception on error
     */
    std::span<char> next_chunk() override;

private:
    io::device& m_dev;
    size_t m_chunk_size = 0;
    std::vector<char> m_buffer;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
