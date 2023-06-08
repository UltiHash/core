#include "fast_cdc.h"

#include "fast_cdc_random.inc"

#include <util/exception.h>

#include <cstring>


namespace uh::chunking
{

// ---------------------------------------------------------------------

fast_cdc::fast_cdc(const fast_cdc_config& c, io::device& in)
        : m_in(in),
          m_geartable(reinterpret_cast<const uint64_t*>(random_gen_table)),
          m_min_size(c.min_size),
          m_max_size(c.max_size),
          m_normal_size(c.normal_size),
          m_buffer(c.max_size),
          m_size(0),
          m_hint(0)
{
    if (!(m_max_size > m_normal_size && m_normal_size > m_min_size))
    {
        THROW(uh::util::illegal_args, "illegal FastCDC limitations");
    }
}

// ---------------------------------------------------------------------

chunk_result fast_cdc::chunk(std::span<char> b)
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

    if (m_hint < m_min_size)
    {
        auto size = m_hint;
        m_hint = 0;
        return { chunk_result::created, size };
    }

    bool done = false;
    unsigned pos = m_min_size;
    for (; pos < std::min(m_hint, m_normal_size) && !done; ++pos)
    {
        m_fp = (m_fp << 1) + m_geartable[b[pos]];
        done = !(m_fp & m_mask_s);
    }

    for (; pos < std::min(m_hint, m_max_size) && !done; ++pos)
    {
        m_fp = (m_fp << 1) + m_geartable[b[pos]];
        done = !(m_fp & m_mask_l);
    }

    m_hint -= pos;
    return { chunk_result::created, pos };
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
