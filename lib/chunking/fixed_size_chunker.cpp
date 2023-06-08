#include "fixed_size_chunker.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

fixed_size_chunker::fixed_size_chunker(io::device& dev,
                                       size_t chunk_size,
                                       std::size_t file_size)
    : m_dev(dev),
      m_chunk_size(chunk_size),
      m_file_size(file_size)
{
}

// ---------------------------------------------------------------------

chunk_result fixed_size_chunker::chunk(std::span<char> b)
{
    std::size_t to_read = std::min(m_chunk_size, m_file_size);
    if (b.size() < to_read)
    {
        return { chunk_result::too_small };
    }

    std::size_t size = m_dev.read(b.subspan(0, to_read));
    if (size == 0)
    {
        return { chunk_result::done };
    }

    m_file_size -= size;
    return { chunk_result::created, size };
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
