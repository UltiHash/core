#include "fast_cdc.h"

#include "fast_cdc_random.inc"

#include <util/exception.h>

#include <cstring>


namespace uh::chunking
{

// ---------------------------------------------------------------------

fast_cdc::fast_cdc(const fast_cdc_config& c)
        : m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
          m_min_size(c.min_size),
          m_max_size(c.max_size),
          m_normal_size(c.normal_size)
{
    if (!(m_max_size > m_normal_size && m_normal_size > m_min_size))
    {
        THROW(uh::util::illegal_args, "illegal FastCDC limitations");
    }
}

// ---------------------------------------------------------------------

std::size_t fast_cdc::next_ofs(std::span<char> b, uint64_t& fp) const
{
    if (b.size() <= m_min_size)
    {
        return b.size();
    }

    bool done = false;
    std::size_t pos = m_min_size;

    for (; pos < std::min(m_normal_size, b.size()) && !done; ++pos)
    {
        fp = (fp << 1) + m_geartable[b[pos]];
        done = !(fp & m_mask_s);
    }

    for (; pos < std::min(m_max_size, b.size()) && !done; ++pos)
    {
        fp = (fp << 1) + m_geartable[b[pos]];
        done = !(fp & m_mask_l);
    }

    return pos;
}

// ---------------------------------------------------------------------

std::vector<std::size_t> fast_cdc::chunk(std::span<char> b) const
{
    std::vector<std::size_t> rv;

    uint64_t fp = 0;
    std::size_t pos = 0;

    while (pos < b.size())
    {
        auto next = next_ofs(b.subspan(pos), fp);

        pos += next;
        rv.push_back(next);
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
