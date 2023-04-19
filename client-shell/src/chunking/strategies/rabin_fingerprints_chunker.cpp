#include "rabin_fingerprints_chunker.h"
#include <vector>


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

rabin_fingerprints_chunker::rabin_fingerprints_chunker(io::device& dev, size_t chunk_size)
    : m_dev(dev),
      m_chunk_size(chunk_size),
      m_buffer(chunk_size)
{
}

// ---------------------------------------------------------------------

std::span<char> rabin_fingerprints_chunker::next_chunk()
{
    std::size_t read = m_dev.read({ m_buffer.data(), m_buffer.size() });

    return { m_buffer.data(), read };
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
