#include "fixed_size_chunker.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

fixed_size_chunker::fixed_size_chunker(size_t chunk_size)
    : m_chunk_size(chunk_size)
{
}

// ---------------------------------------------------------------------

std::vector<std::size_t> fixed_size_chunker::chunk(std::span<char> b) const
{
    std::vector<std::size_t> rv;

    for (std::size_t pos = 0u; pos < b.size(); pos += m_chunk_size)
    {
        rv.push_back(std::min(m_chunk_size, b.size() - pos));
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
