#include "gear.h"

#include "gear_random.inc"

#include <cstring>


namespace uh::chunking
{

namespace
{

// ---------------------------------------------------------------------

/*
 * Defines average chunk size: number of most significant bits set is equal
 * to log(average chunk size). Here: log(8192) = 13.
 */
uint64_t compute_mask(std::size_t average_size)
{
    uint64_t mask = 0;

    constexpr uint64_t msb = ((~0ul) >> (8*sizeof(uint64_t)-1)) << (8*sizeof(uint64_t)-1);

    while (average_size)
    {
        mask = (mask >> 1) | msb;
        average_size = average_size >> 1;
    }

    return mask;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

gear::gear(const gear_config& c)
    : m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
      m_max_size(c.max_size),
      m_mask(compute_mask(c.average_size))
{
}

// ---------------------------------------------------------------------

std::vector<uint32_t> gear::chunk(std::span<char> b) const
{
    std::vector<uint32_t> rv;

    uint64_t fp = 0;
    std::size_t pos = 0u;
    std::size_t last = 0u;

    for (pos = 0u; pos < b.size(); ++pos)
    {
        fp = (fp << 1) + m_geartable[b[pos]];
        if ((fp & m_mask) == 0)
        {
            rv.push_back(pos - last);
            last = pos;
        }
    }

    if (last != pos)
    {
        rv.push_back(pos - last);
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
