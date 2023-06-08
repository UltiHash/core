#include "rabin_fp.h"

#include <util/exception.h>

#include <cstring>
#include <vector>


namespace uh::chunking
{

// ---------------------------------------------------------------------

rabin_fp::rabin_fp(const rabin_fp_config& config, io::device& dev)
    : m_dev(dev),
      m_block(nullptr),
      m_chunk(nullptr),
      m_buffer(config.read_buf_size),
      m_max_size(config.read_buf_size),
      m_size(0),
      m_hint(0)
{
    static int __rabin_init_result = initialize_rabin_polynomial_defaults();

    if(!__rabin_init_result)
    {
        throw(std::runtime_error("Error initializing Rabin fingerprints"));
    }
}

// ---------------------------------------------------------------------

rabin_fp::~rabin_fp()
{
    free_chunk_data(m_block);
    free_rabin_fingerprint_list(m_block->head);
    free(m_block);
}

// ---------------------------------------------------------------------

chunk_result rabin_fp::chunk(std::span<char> b)
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

        m_block = read_rabin_block(b.data(), m_hint, m_block);
        if (!m_chunk)
        {
            m_chunk = m_block->head;
        }
    }

    ASSERT(m_chunk->chunk_data == b.data());

    auto size = m_chunk->length;
    m_hint -= m_chunk->length;
    m_chunk = m_chunk->next_polynomial;
    return { chunk_result::created, size };
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
