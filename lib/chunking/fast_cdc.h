#ifndef CHUNKING_FAST_CDC_H
#define CHUNKING_FAST_CDC_H

#include <io/device.h>
#include <chunking/buffer.h>
#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

class fast_cdc : public chunker
{
public:
    fast_cdc(io::device& in, std::size_t min_size, std::size_t max_size);

    std::span<char> next_chunk() override;

private:
    void to_split_border();

    buffer m_buffer;

    uint64_t m_fp = 0;
    const uint64_t* m_geartable;
    std::size_t m_min_size;
    std::size_t m_max_size;
    std::size_t m_normal_size;

    static constexpr uint64_t m_mask_s = 0x0003590703530000;
    static constexpr uint64_t m_mask_l = 0x0000d90303530000;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
