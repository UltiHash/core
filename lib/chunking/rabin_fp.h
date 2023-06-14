#ifndef CHUNKING_RABIN_FINGERPRINTS_H
#define CHUNKING_RABIN_FINGERPRINTS_H

#include <chunking/chunker.h>


namespace uh::chunking
{

// ---------------------------------------------------------------------

class rabin_fp : public chunker
{
public:
    rabin_fp();

    std::vector<std::size_t> chunk(std::span<char> b) const override;

    std::size_t min_size() const override { return 0u; }
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
