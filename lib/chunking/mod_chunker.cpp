//
// Created by masi on 4/28/23.
//
#include "mod_chunker.h"
#include "util/exception.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

mod_chunker::mod_chunker(const uh::chunking::mod_cdc_config& config)
    : m_min_size(config.min_size),
      m_max_size(config.max_size),
      m_normal_size(config.normal_size)
{
}

// ---------------------------------------------------------------------

std::size_t mod_chunker::next_ofs(std::span<char> b) const
{
    const auto mask = m_normal_size - m_min_size;
    unsigned long window = 0;

    if (b.size() < m_min_size)
    {
        return b.size();
    }

    auto pos = m_min_size;
    for (; pos < std::min(m_max_size, b.size()); ++pos)
    {
        window = (window << 1) + b[pos];
        if (window % mask == 0)
        {
            break;
        }
    }

    return pos;
}

// ---------------------------------------------------------------------

std::vector<uint32_t> mod_chunker::chunk(std::span<char> b) const
{
    std::vector<uint32_t> rv;

    std::size_t pos = 0;

    while (pos < b.size())
    {
        auto next = next_ofs(b.subspan(pos));

        pos += next;
        rv.push_back(next);
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
