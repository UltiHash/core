#ifndef CLIENT_CHUNKING_FIXED_SIZE_H
#define CLIENT_CHUNKING_FIXED_SIZE_H

#include <io/device.h>
#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

class fixed_size_chunker : public uh::chunking::chunker
{
public:
    fixed_size_chunker(io::device& dev,
                       std::size_t chunk_size,
                       std::size_t file_size);

    chunk_result chunk(std::span<char> b) override;

private:
    io::device& m_dev;
    std::size_t m_chunk_size;
    std::size_t m_file_size;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
