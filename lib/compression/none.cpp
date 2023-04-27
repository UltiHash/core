#include "none.h"


namespace uh::comp
{

// ---------------------------------------------------------------------

none::none(io::device& base)
    : m_base(base)
{
}

// ---------------------------------------------------------------------

std::streamsize none::write(std::span<const char> buffer)
{
    return m_base.write(buffer);
}

// ---------------------------------------------------------------------

std::streamsize none::read(std::span<char> buffer)
{
    return m_base.read(buffer);
}

// ---------------------------------------------------------------------

bool none::valid() const
{
    return m_base.valid();
}

// ---------------------------------------------------------------------

} // namespace uh::comp
