#ifndef COMPRESSION_BROTLI_H
#define COMPRESSION_BROTLI_H

#include <io/device.h>
#include <memory>

#include <brotli/decode.h>
#include <brotli/encode.h>


namespace uh::comp
{

// ---------------------------------------------------------------------

class brotli : public io::device
{
public:
    brotli(io::device& base);
    ~brotli();

    std::streamsize write(std::span<const char> buffer) override;
    std::streamsize read(std::span<char> buffer) override;
    bool valid() const override;

    static constexpr std::size_t BUFFER_SIZE = 64 * 1024;

private:
    BrotliEncoderState* enc() { return m_enc_state.get(); }
    BrotliDecoderState* dec() { return m_dec_state.get(); }
    void finish();

    std::unique_ptr<BrotliEncoderState, void(*)(BrotliEncoderState*)> m_enc_state;
    std::unique_ptr<BrotliDecoderState, void(*)(BrotliDecoderState*)> m_dec_state;
    io::device& m_base;

    std::vector<char> m_in_buffer;
    const uint8_t* m_in_next;
    std::size_t m_in_available;

    BrotliDecoderResult m_dec_result;
};

// ---------------------------------------------------------------------

} // namespace uh::comp

#endif
