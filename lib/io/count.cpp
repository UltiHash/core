#include "count.h"


namespace uh::io
{

// ---------------------------------------------------------------------

count::count(device& base)
    : m_base(base)
{
}

// ---------------------------------------------------------------------

std::streamsize count::write(std::span<const char> buffer)
{
    auto rv = m_base.write(buffer);
    m_written += rv;
    return rv;
}

// ---------------------------------------------------------------------

std::streamsize count::read(std::span<char> buffer)
{
    auto rv = m_base.read(buffer);
    m_read += rv;
    return rv;
}

// ---------------------------------------------------------------------

bool count::valid() const
{
    return m_base.valid();
}

// ---------------------------------------------------------------------

std::streamsize count::written() const
{
    return m_written;
}

// ---------------------------------------------------------------------

std::streamsize count::read() const
{
    return m_read;
}

// ---------------------------------------------------------------------

} // namespace uh::io
