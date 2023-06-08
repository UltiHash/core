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

gear::gear(const gear_config& c, io::device& in)
    : m_in(in),
      m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
      m_max_size(c.max_size),
      m_mask(compute_mask(c.average_size)),
      m_buffer(c.max_size),
      m_size(0u),
      m_hint(0u)
{
}

// ---------------------------------------------------------------------

chunk_result gear::chunk(std::span<char> b)
{
    if (b.size() < m_max_size)
    {
        memcpy(&m_buffer[0], b.data(), m_hint);
        m_size = m_hint;
        m_hint = 0;

        return { chunk_result::too_small };
    }

    if (m_hint == 0)
    {
        memcpy(b.data(), &m_buffer[0], m_size);

        m_hint = m_in.read(b.subspan(m_size));
        m_hint = m_hint + m_size;
        m_size = 0;

        if (m_hint == 0)
        {
            return { chunk_result::done };
        }
    }

    auto pos = 0u;
    for (; pos < m_hint; ++pos)
    {
        m_fp = (m_fp << 1) + m_geartable[b[pos]];
        if ((m_fp & m_mask) == 0)
        {
            break;
        }
    }

    m_hint -= pos;
    return { chunk_result::created, pos };
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
