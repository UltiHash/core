#include "data_generator.h"

#include <util/exception.h>


namespace uh::io
{

// ---------------------------------------------------------------------

data_chunk::data_chunk(std::vector<char>&& buffer)
    : m_buffer(std::move(buffer)),
      m_span()
{
}

// ---------------------------------------------------------------------

data_chunk::data_chunk(std::span<char> span)
    : m_buffer(),
      m_span(span)
{
}

// ---------------------------------------------------------------------

std::span<char> data_chunk::data()
{
    if (!m_buffer.empty())
    {
        return m_buffer;
    }

    if (!m_span.empty())
    {
        return m_span;
    }

    THROW(util::illegal_state, "data_chunk all buffers empty");
}

// ---------------------------------------------------------------------

} // namespace uh::io
