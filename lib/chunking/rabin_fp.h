#ifndef CHUNKING_RABIN_FINGERPRINTS_H
#define CHUNKING_RABIN_FINGERPRINTS_H

#include <io/device.h>
#include <chunking/chunker.h>

extern "C"
{

#include <chunking/rabin_polynomial.h>
#include <chunking/rabin_polynomial_constants.h>

}


namespace uh::chunking
{

// ---------------------------------------------------------------------

struct rabin_fp_config
{
    //# of bytes to read at a time when reading through files
    std::size_t read_buf_size = 512;
};

// ---------------------------------------------------------------------

class rabin_fp : public chunker
{
public:
    rabin_fp(const rabin_fp_config& config, io::device& dev);

    ~rabin_fp() override;

    chunk_result chunk(std::span<char> b) override;

private:
    io::device& m_dev;
    struct rab_block_info *m_block=nullptr;
    struct rabin_polynomial *m_chunk=nullptr;
    std::vector<char> m_buffer;
    std::size_t m_max_size;
    std::size_t m_size;
    std::size_t m_hint;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
