#include "fast_cdc.h"

#include "fast_cdc_random.inc"


namespace uh::chunking
{

// ---------------------------------------------------------------------

fast_cdc::fast_cdc(io::device& in, std::size_t min_size, std::size_t max_size)
    : m_buffer(in, max_size),
      m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
      m_min_size(min_size),
      m_max_size(max_size),
      m_normal_size(8 * 1024)
{
}

// ---------------------------------------------------------------------

std::span<char> fast_cdc::next_chunk()
{
    if (m_buffer.fill_buffer() == 0)
    {
        return m_buffer.data();
    }

    if (m_buffer.length() < m_min_size)
    {
        return m_buffer.data();
    }

    auto start = m_buffer.mark();
    to_split_border();
    return m_buffer.data(start);
}

// ---------------------------------------------------------------------

void fast_cdc::to_split_border()
{
    auto normal = m_normal_size;
    if (m_buffer.length() < normal)
    {
        normal = m_buffer.length();
    }

    m_buffer.skip(m_min_size);
    unsigned pos = m_min_size;

    for (; pos < normal; ++pos)
    {
        int ch = m_buffer.next_byte();
        m_fp = (m_fp << 1) + m_geartable[ch];

        if (!(m_fp & m_mask_s))
        {
            return;
        }
    }

    for (; pos < m_buffer.length(); ++pos)
    {
        int ch = m_buffer.next_byte();
        m_fp = (m_fp << 1) + m_geartable[ch];

        if (!(m_fp & m_mask_l))
        {
            return;
        }
    }
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
