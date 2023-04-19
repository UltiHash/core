#include "fixed_size_chunker.h"


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

fixed_size_chunker::fixed_size_chunker(io::device& dev, size_t chunk_size)
    : m_dev(dev),
      m_chunk_size(chunk_size),
      m_buffer(chunk_size)
{
}

// ---------------------------------------------------------------------

std::span<char> fixed_size_chunker::next_chunk()
{
    std::size_t read = m_dev.read({ m_buffer.data(), m_buffer.size() });

    return { m_buffer.data(), read };
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
