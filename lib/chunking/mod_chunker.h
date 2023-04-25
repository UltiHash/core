//
// Created by masi on 4/24/23.
//

#ifndef CORE_CHUNKTEST_H
#define CORE_CHUNKTEST_H

#include <cstring>
#include "chunker.h"
#include "io/device.h"


namespace uh::chunking {

struct mod_cdc_config
{
    /**
     * Minimum size of chunks.
     */
    std::size_t min_size = 4 * 1024;

    /**
     * Maximum size of chunks.
     */
    std::size_t max_size = 8 * 1024;

    /**
     * Normalized size of chunks. max_size > normal_size > min_size.
     */
    std::size_t normal_size = 6 * 1024;
};

class mod_chunker : public chunker {
public:
    mod_chunker(const mod_cdc_config &config, io::device &in)
            :
            m_min_size(config.min_size),
            m_max_size(config.max_size),
            m_normal_size(config.normal_size),
            m_dev (in),
            m_buffer (new char [m_max_size * 10]),
            m_buf_size (m_dev.read({m_buffer, m_max_size * 10}))

    {
    }

    std::span<char> next_chunk() override {
        if (m_buf_size - m_pointer < m_max_size) {
            refresh_buffer();
            if (m_buf_size < m_max_size) {
                return {m_buffer, m_buf_size};
            }
        }

        const auto start = m_pointer;
        const auto loop_limit = std::min (start + m_max_size, m_buf_size) - sizeof (unsigned long);
        for (auto i = m_pointer + m_min_size; i < loop_limit; i++) {
            const auto data = *reinterpret_cast <unsigned long *> (m_buffer + i);
            if (data % m_normal_size == 0) {
                m_pointer = i + 1;
                return {m_buffer + start, i - start};
            }
        }

        m_pointer = loop_limit + 1;
        return {m_buffer + start, loop_limit - start};

    }

    ~mod_chunker() override {
        delete [] m_buffer;
    }
private:

    inline void refresh_buffer () {
        const auto remaining_size = m_buf_size - m_pointer;
        std::memmove(m_buffer, m_buffer + m_pointer, remaining_size);
        m_buf_size = m_dev.read({m_buffer + remaining_size, m_buf_size - remaining_size});
        m_pointer = 0;
    }
    const std::size_t m_min_size;
    const std::size_t m_max_size;
    const std::size_t m_normal_size;
    io::device &m_dev;
    char *m_buffer;
    std::size_t m_buf_size;
    std::size_t m_pointer {};


};

}
#endif //CORE_CHUNKTEST_H
