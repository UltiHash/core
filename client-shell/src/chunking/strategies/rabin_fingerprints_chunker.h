#ifndef CLIENT_CHUNKING_RABIN_FINGERPRINTS_H
#define CLIENT_CHUNKING_RABIN_FINGERPRINTS_H

#include "../file_chunker.h"
#include <io/device.h>
extern "C"{
    #include <chunking/rabin_polynomial.h>
    #include <chunking/rabin_polynomial_constants.h>
}


namespace uh::client::chunking
{

// ---------------------------------------------------------------------

class rabin_fingerprints_chunker : public file_chunker
{
public:
    rabin_fingerprints_chunker(io::device& dev, size_t chunk_size);

    /**
     * Return the next chunk to upload. If there are no more chunks, return
     * an empty chunk instead.
     *
     * @throw may throw any derivative of exception on error
     */
    std::span<char> next_chunk() override;

    std::string chunker_type() {return std::string(m_type);}

    size_t refill_buffer();

private:
    io::device& m_dev;
    size_t m_chunk_size = 0;
    std::vector<char> m_buffer;
    struct rab_block_info *m_block=nullptr;
    struct rabin_polynomial *m_chunk=nullptr;
    constexpr static std::string_view m_type = "CDCrabin";
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
