#ifndef CHUNKING_GEAR_H
#define CHUNKING_GEAR_H

#include <chunking/buffer.h>
#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

class gear : public chunker
{
public:
    gear(io::device& in, std::size_t max_size);

    std::span<char> next_chunk() override;

private:
    buffer m_buffer;

    uint64_t m_fp = 0;
    const uint64_t* m_geartable;
    std::size_t m_max_size;

    /*
     * Defines average chunk size: number of most significant bits set is equal
     * to log(average chunk size). Here: log(8192) = 13.
     */
    static constexpr uint64_t significant_bits = 18;
    static constexpr uint64_t m_mask = (~0ul) << (64 - significant_bits);
};
// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
