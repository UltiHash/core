//
// Created by masi on 4/28/23.
//
#include "mod_chunker.h"
#include "util/exception.h"


namespace uh::chunking
{

// ---------------------------------------------------------------------

mod_chunker::mod_chunker(const uh::chunking::mod_cdc_config& config, uh::io::device& in)
        :
        m_min_size(config.min_size),
        m_max_size(config.max_size),
        m_normal_size(config.normal_size),
        m_dev (in),
        m_buffer(config.max_size),
        m_size(0),
        m_hint(0)
{
}

// ---------------------------------------------------------------------

chunk_result mod_chunker::chunk(std::span<char> b)
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

        m_hint = m_dev.read(b.subspan(m_size));
        m_hint = m_hint + m_size;
        m_size = 0;

        if (m_hint == 0)
        {
            return { chunk_result::done };
        }
    }

    const auto mask = m_normal_size - m_min_size;
    unsigned long window = 0;

    if (m_hint < m_min_size)
    {
        auto size = m_hint;
        m_hint = 0;
        return { chunk_result::created, size };
    }

    auto pos = m_min_size;
    for (; pos < std::min(m_max_size, m_hint); ++pos)
    {
        window = (window << 1) + b[pos];
        if (window % mask == 0)
        {
            break;
        }
    }

    m_hint -= pos;
    return { chunk_result::created, pos };
}

// ---------------------------------------------------------------------

} // namespace uh::chunking
