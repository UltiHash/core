#ifndef CHUNKING_RABIN_FINGERPRINTS_H
#define CHUNKING_RABIN_FINGERPRINTS_H

#include <io/device.h>
#include <chunking/buffer.h>
#include <chunking/chunker.h>
extern "C"{
    #include <chunking/rabin_polynomial.h>
    #include <chunking/rabin_polynomial_constants.h>
}


namespace uh::chunking
{

// ---------------------------------------------------------------------

struct rabin_fingerprints_config
{
    std::size_t chunk_size = 1024;
};

// ---------------------------------------------------------------------

class rabin_fingerprints_chunker : public chunker
{
public:
    rabin_fingerprints_chunker(const rabin_fingerprints_config& config, io::device& dev);

    ~rabin_fingerprints_chunker() override;

    std::span<char> next_chunk() override;

    std::string chunker_type() {return std::string(m_type);}

    size_t refill_buffer();

private:
    io::device& m_dev;
    size_t m_chunk_size = 0;
    std::vector<char> m_buffer;
    struct rab_block_info *m_block=nullptr;
    struct rabin_polynomial *m_chunk=nullptr;
    bool m_update_chunk=false;
    bool m_finished=false;
    constexpr static std::string_view m_type = "CDCrabin";
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
