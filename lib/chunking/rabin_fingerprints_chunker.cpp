#include "rabin_fingerprints_chunker.h"
#include <logging/logging_boost.h>
#include <vector>
#include <iostream>


namespace uh::chunking
{

// ---------------------------------------------------------------------

rabin_fingerprints_chunker::rabin_fingerprints_chunker(io::device& dev, size_t chunk_size)
    : m_dev(dev),
      m_chunk_size(chunk_size),
      m_buffer(chunk_size),
      m_block(nullptr)
{
    initialize_rabin_polynomial_defaults();
    INFO << "--- Storage backend initialized --- ";
    INFO << "        chunking strategy : " << chunker_type();
}

// ---------------------------------------------------------------------

size_t rabin_fingerprints_chunker::refill_buffer()
{
    size_t bytes_read=m_dev.read({m_buffer.data(), m_buffer.size()});

    if(!bytes_read)
        return bytes_read;

    m_block=read_rabin_block(m_buffer.data(), bytes_read, m_block);

    if(!m_chunk){
        m_chunk=m_block->head;
    }
    else{
        m_chunk=m_chunk->next_polynomial;
    }

    return bytes_read;
}

std::span<char> rabin_fingerprints_chunker::next_chunk()
{
    if(!m_chunk){
        if(!refill_buffer())
            return {};
    }
    else
    {
        m_chunk = m_chunk->next_polynomial;
    }
    if(!m_chunk->next_polynomial){
        if(!refill_buffer())
            return {};
    }

    return {m_chunk->chunk_data, m_chunk->length};
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
