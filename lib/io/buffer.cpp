#include "buffer.h"

#include <cstring>


namespace uh::io
{

// ---------------------------------------------------------------------

buffer::buffer(std::size_t size)
    : m_buffer(size),
      m_rptr(0u),
      m_wptr(0u)
{
}

// ---------------------------------------------------------------------

std::streamsize buffer::write(std::span<const char> buffer)
{
    std::size_t count = std::min(m_buffer.size() - m_wptr, buffer.size());

    memcpy(&m_buffer[m_wptr], buffer.data(), count);
    m_wptr += count;

    return count;
}

// ---------------------------------------------------------------------

std::streamsize buffer::read(std::span<char> buffer)
{
    std::size_t count = std::min(m_wptr - m_rptr, buffer.size());

    memcpy(buffer.data(), &m_buffer[m_rptr], count);
    m_rptr += count;

    return count;
}

// ---------------------------------------------------------------------

bool buffer::valid() const
{
    return m_rptr < m_wptr;
}

// ---------------------------------------------------------------------

const std::vector<char> &buffer::get_buffer() {
    return m_buffer;
}

// ---------------------------------------------------------------------

} // namespace uh::io
