#ifndef CLIENT_CHUNKING_FIXED_SIZE_H
#define CLIENT_CHUNKING_FIXED_SIZE_H

#include <io/device.h>
#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

class fixed_size_chunker : public uh::chunking::chunker
{
public:
    fixed_size_chunker(std::size_t chunk_size);

    std::vector<uint32_t> chunk(std::span<char> b) const override;

    std::size_t min_size() const override { return 1u; }
private:
    std::size_t m_chunk_size;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
