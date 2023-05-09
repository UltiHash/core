#include "fixed_size_chunker.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

fixed_size_chunker::fixed_size_chunker(io::device& dev, size_t chunk_size, size_t buffer_size)
    : m_chunk_size(chunk_size),
      m_buffer(dev, std::max (buffer_size, 2 * chunk_size))
{
}

// ---------------------------------------------------------------------

std::span<char> fixed_size_chunker::next_chunk()
{
    if (m_buffer.fill_buffer() == 0)
    {
        return m_buffer.data();
    }

    if (m_buffer.length() < m_chunk_size)
    {
        auto start = m_buffer.mark();
        m_buffer.skip(m_buffer.length());
        return m_buffer.data(start);
    }

    auto start = m_buffer.mark();
    m_buffer.skip(m_chunk_size);
    return m_buffer.data(start);
}

// ---------------------------------------------------------------------

buffer &fixed_size_chunker::get_buffer() {
    return m_buffer;
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
