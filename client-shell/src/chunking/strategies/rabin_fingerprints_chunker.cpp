#include "rabin_fingerprints_chunker.h"
#include <vector>


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

rabin_fingerprints_chunker::rabin_fingerprints_chunker(io::device& dev, size_t chunk_size)
    : m_dev(dev),
      m_chunk_size(chunk_size),
      m_buffer(chunk_size),
      m_block(nullptr)
{
    initialize_rabin_polynomial_defaults();
    //JM  - TODO - is this init state going to persist? Do we need to fix it?
}

// ---------------------------------------------------------------------

std::span<char> rabin_fingerprints_chunker::next_chunk()
{
///
    //file_data is the content of each chunk
    //a block contains several chunks positions but not data itself
    //a block grows with chunk positions and hashes at each loop, but the data is thrown away
    size_t bytes_read=m_dev.read({m_buffer.data(), m_buffer.size()});
    m_block=read_rabin_block(m_buffer.data(), bytes_read, m_block);
///

    return { m_buffer.data(), bytes_read };
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
