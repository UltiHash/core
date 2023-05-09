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

    std::span<char> next_chunk() override;

    [[nodiscard]] buffer& get_buffer () override;

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
