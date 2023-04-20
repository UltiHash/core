#include "gear.h"

#include "gear_random.inc"


namespace uh::chunking
{

// ---------------------------------------------------------------------

gear::gear(io::device& in, std::size_t max_size)
    : m_buffer(in, max_size),
      m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
      m_max_size(max_size)
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

} // namespace uh::chunking
