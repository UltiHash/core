#include "fast_cdc.h"

#include "fast_cdc_random.inc"

#include <util/exception.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

fast_cdc::fast_cdc(const fast_cdc_config& c, io::device& in)
    : m_buffer(in, c.max_size),
      m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
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

std::span<char> fast_cdc::next_chunk()
{
    if (m_buffer.fill_buffer() == 0)
    {
        return m_buffer.data();
    }

    if (m_buffer.length() < m_min_size)
    {
        auto start = m_buffer.mark();
        m_buffer.skip(m_buffer.length());
        return m_buffer.data(start);
    }

    auto start = m_buffer.mark();
    to_split_border();
    return m_buffer.data(start);
}

// ---------------------------------------------------------------------

void fast_cdc::to_split_border()
{
    auto normal = std::min(m_normal_size, m_buffer.length());

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

    for (; pos < std::min(m_max_size, m_buffer.length()); ++pos)
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
