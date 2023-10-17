#include "buffer_generator.h"


namespace uh::io
{

// ---------------------------------------------------------------------

buffer_generator::buffer_generator(std::vector<char>&& b)
    : m_buffer(std::move(b))
{
}

// ---------------------------------------------------------------------

std::optional<data_chunk> buffer_generator::next()
{
    if (m_buffer.empty())
    {
        return std::nullopt;
    }

    return data_chunk(std::move(m_buffer));
}

// ---------------------------------------------------------------------

std::size_t buffer_generator::size() const
{
    return m_buffer.size();
}

// ---------------------------------------------------------------------

} // namespace uh::io
