#ifndef CHUNKING_FAST_CDC_H
#define CHUNKING_FAST_CDC_H

#include <io/device.h>
#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

struct fast_cdc_config
{
    /**
     * Minimum size of chunks.
     */
    std::size_t min_size = 8192;

    /**
     * Maximum size of chunks.
     */
    std::size_t max_size = 1 * 1024 * 1024;

    /**
     * Normalized size of chunks. max_size > normal_size > min_size.
     */
    std::size_t normal_size = 128 * 1024;
};

// ---------------------------------------------------------------------

class fast_cdc : public chunker
{
public:
    fast_cdc(const fast_cdc_config& config);

    std::vector<std::size_t> chunk(std::span<char> b) const override;

    std::size_t min_size() const override { return m_min_size; }

private:
    std::size_t next_ofs(std::span<char> b, uint64_t& fp) const;

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
