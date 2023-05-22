#include "data_generator.h"

#include <util/exception.h>


namespace uh::io
{

// ---------------------------------------------------------------------

data_chunk::data_chunk(std::vector<char>&& buffer)
    : m_data(std::move(buffer))
{
}

// ---------------------------------------------------------------------

data_chunk::data_chunk(std::span<const char> span)
    : m_data(span)
{
}

// ---------------------------------------------------------------------

std::span<const char> data_chunk::data()
{
    switch (m_data.index())
    {
        case 0: return std::get<0>(m_data);
        case 1: return std::get<1>(m_data);
    }

    THROW(util::illegal_state, "data_chunk all buffers empty");
}

// ---------------------------------------------------------------------

} // namespace uh::io
