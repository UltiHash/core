//
// Created by masi on 4/28/23.
//
#include "mod_chunker.h"

uh::chunking::mod_chunker::mod_chunker(const uh::chunking::mod_cdc_config &config, uh::io::device &in, size_t chunks_count_in_buffer)
        :
        m_min_size(config.min_size),
        m_max_size(config.max_size),
        m_normal_size(config.normal_size),
        m_dev (in),
        m_buffer (m_max_size * chunks_count_in_buffer),
        m_buf_size (m_dev.read({m_buffer.data(), m_buffer.size()}))

{
}

std::span<char> uh::chunking::mod_chunker::next_chunk() {
    if (m_buf_size - m_pointer < m_max_size) {
        refresh_buffer();
        if (m_buf_size < m_max_size) {
            m_pointer = m_buf_size;
            return {m_buffer.data(), m_buf_size};
        }
    }

    const auto start = m_pointer;
    const auto loop_limit = std::min (start + m_max_size, m_buf_size);
    unsigned long window = 0;
    for (auto i = m_pointer + m_min_size; i < loop_limit; i++) {
        window = (window << 1) + m_buffer [i];
        if (window % m_normal_size == 0) {
            m_pointer = i;
            return {m_buffer.data() + start, i - start};
        }
    }

    m_pointer = loop_limit;
    return {m_buffer.data() + start, loop_limit - start};

}

void uh::chunking::mod_chunker::refresh_buffer() {
    const auto remaining_size = m_buf_size - m_pointer;
    std::memmove(m_buffer.data(), m_buffer.data() + m_pointer, remaining_size);
    m_buf_size = m_dev.read({m_buffer.data() + remaining_size, m_buf_size - remaining_size}) + remaining_size;
    m_pointer = 0;
}


