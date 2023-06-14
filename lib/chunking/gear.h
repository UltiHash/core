#ifndef CHUNKING_GEAR_H
#define CHUNKING_GEAR_H

#include <io/device.h>
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
    gear(const gear_config& c);

    std::vector<std::size_t> chunk(std::span<char> b) const override;

    std::size_t min_size() const override { return 0u; }

private:
    const uint64_t* m_geartable;
    std::size_t m_max_size;
    uint64_t m_mask;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
