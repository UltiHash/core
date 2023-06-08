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
    mod_chunker(const mod_cdc_config &config, io::device &in);

    chunk_result chunk(std::span<char> b) override;

private:
    const std::size_t m_min_size;
    const std::size_t m_max_size;
    const std::size_t m_normal_size;
    io::device &m_dev;

    std::vector<char> m_buffer;
    std::size_t m_size;
    std::size_t m_hint;
};

}

#endif //CORE_CHUNKTEST_H
