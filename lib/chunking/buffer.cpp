#include "buffer.h"

#include <cstring>


namespace uh::chunking
{

// ---------------------------------------------------------------------

buffer::buffer(io::device& in, std::size_t size)
    : m_in(in),
        m_buffer(2 * size),
        m_wptr(0),
        m_rptr(0),
        m_size(size)
{
}

// ---------------------------------------------------------------------

std::size_t buffer::fill_buffer()
{
    if (m_rptr > m_size)
    {
        std::memmove(&m_buffer[0], &m_buffer[m_rptr], m_wptr - m_rptr);
        m_wptr = m_wptr - m_rptr;
        m_rptr = 0;
    }

    m_wptr += m_in.read({ &m_buffer[m_wptr], m_buffer.size() - m_wptr });

    return length();
}

// ---------------------------------------------------------------------

int buffer::next_byte()
{
    if (m_rptr == m_wptr)
    {
        return -1;
    }

    return (unsigned char)m_buffer[m_rptr++];
}

// ---------------------------------------------------------------------

void buffer::skip(std::size_t count)
{
    m_rptr += count;
}

// ---------------------------------------------------------------------

std::size_t buffer::length() const
{
    return m_wptr - m_rptr;
}

// ---------------------------------------------------------------------

buffer::pos::pos(std::size_t start)
    : m_start(start)
{
}

// ---------------------------------------------------------------------

buffer::pos buffer::mark() const
{
    return m_rptr;
}

// ---------------------------------------------------------------------

std::size_t buffer::length(const pos& p) const
{
    return m_rptr - p.m_start;
}

// ---------------------------------------------------------------------

std::span<char> buffer::data(const pos& p)
{
    return { &m_buffer[p.m_start], m_rptr - p.m_start };
}

// ---------------------------------------------------------------------

std::span<char> buffer::data()
{
    return { &m_buffer[m_rptr], length() };
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
