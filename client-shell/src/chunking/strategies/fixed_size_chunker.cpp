#include "fixed_size_chunker.h"


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

fixed_size_chunker::fixed_size_chunker(io::device& dev, size_t chunk_size)
    : m_dev(dev),
      m_chunk_size(chunk_size)
{
}

// ---------------------------------------------------------------------

uh::protocol::blob fixed_size_chunker::next_chunk()
{
    uh::protocol::blob chunk(m_chunk_size);

    auto read = m_dev.read({ chunk.data(), chunk.size() });
    chunk.resize(read);

    return chunk;
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
