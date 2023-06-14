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
    mod_chunker(const mod_cdc_config &config);

    std::vector<std::size_t> chunk(std::span<char> b) const override;

    std::size_t min_size() const override { return m_min_size; }

private:
    std::size_t next_ofs(std::span<char> b) const;

    const std::size_t m_min_size;
    const std::size_t m_max_size;
    const std::size_t m_normal_size;
};

}

#endif //CORE_CHUNKTEST_H
