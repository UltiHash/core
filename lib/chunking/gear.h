#ifndef CHUNKING_GEAR_H
#define CHUNKING_GEAR_H

#include <chunking/buffer.h>
#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

struct gear_config
{
    /**
     * Maximum size of produced chunks.
     */
    std::size_t max_size = 128 * 1024;

    /**
     * Average size of chunks.
     */
    std::size_t average_size = 16 * 1024;
};

// ---------------------------------------------------------------------

class gear : public chunker
{
public:
    gear(const gear_config& c, io::device& in);

    std::span<char> next_chunk() override;

private:
    buffer m_buffer;

    uint64_t m_fp = 0;
    const uint64_t* m_geartable;
    std::size_t m_max_size;
    uint64_t m_mask;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
