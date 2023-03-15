#include "buffer.h"

#include <cstring>


namespace uh::io
{

// ---------------------------------------------------------------------

buffer::buffer(std::size_t size)
    : m_buffer(size),
      m_wptr(0u),
      m_rptr(0u)
{
}

// ---------------------------------------------------------------------

std::streamsize buffer::write(std::span<const char> buffer)
{
    auto count = std::min(buffer.size(), m_buffer.size() - m_wptr);

    memcpy(&m_buffer[m_wptr], buffer.data(), count);
    m_wptr += count;

    return count;
}

// ---------------------------------------------------------------------

std::streamsize buffer::read(std::span<char> buffer)
{
    auto count = std::min(buffer.size(), m_wptr - m_rptr);

    memcpy(buffer.data(), &m_buffer[m_rptr], count);
    m_rptr += count;

    return count;
}

// ---------------------------------------------------------------------

bool buffer::valid() const
{
    return m_rptr < m_buffer.size();
}

// ---------------------------------------------------------------------

} // namespace uh::io
