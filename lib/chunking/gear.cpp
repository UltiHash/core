#include "gear.h"

#include "gear_random.inc"


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

gear::gear(const gear_config& c, io::device& in, std::size_t buffer_size)
    : m_buffer(in, std::max (buffer_size, 2 * c.max_size)),
      m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
      m_max_size(c.max_size),
      m_mask(compute_mask(c.average_size))
{
}

// ---------------------------------------------------------------------

std::span<char> gear::next_chunk()
{
    if (m_buffer.fill_buffer() == 0)
    {
        return m_buffer.data(m_buffer.mark());
    }

    auto start = m_buffer.mark();
    int ch;
    while ((ch = m_buffer.next_byte()) != -1)
    {
        m_fp = (m_fp << 1) + m_geartable[ch];
        if ((m_fp & m_mask) == 0)
        {
            break;
        }

        if (m_buffer.length(start) >= m_max_size)
        {
            break;
        }
    }

    return m_buffer.data(start);
}

// ---------------------------------------------------------------------

buffer &gear::get_buffer() {
    return m_buffer;
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
