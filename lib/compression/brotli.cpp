#include "brotli.h"

#include <util/exception.h>


namespace uh::comp
{

// ---------------------------------------------------------------------

brotli::brotli(io::device& base)
    : m_enc_state(BrotliEncoderCreateInstance(nullptr, nullptr, nullptr),
                  BrotliEncoderDestroyInstance),
      m_dec_state(BrotliDecoderCreateInstance(nullptr, nullptr, nullptr),
                  BrotliDecoderDestroyInstance),
      m_base(base),
      m_in_buffer(BUFFER_SIZE),
      m_in_next(nullptr),
      m_in_available(0u),
      m_dec_result(BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
{
}

// ---------------------------------------------------------------------

brotli::~brotli()
{
    finish();
}

// ---------------------------------------------------------------------

std::streamsize brotli::write(std::span<const char> buffer)
{
    std::size_t available_in = buffer.size();
    const uint8_t* next_in = reinterpret_cast<const uint8_t*>(buffer.data());

    std::vector<uint8_t> out_buffer(brotli::BUFFER_SIZE);
    uint8_t* out_next = out_buffer.data();
    std::size_t out_avail = out_buffer.size();

    while (available_in > 0)
    {
        if (BrotliEncoderCompressStream(
            enc(),
            BROTLI_OPERATION_PROCESS,
            &available_in,
            &next_in,
            &out_avail,
            &out_next,
            nullptr) == BROTLI_FALSE)
        {
            THROW(util::exception, "BrotliEncoderCompressStream(BROTLI_OPERATION_PROCESS) failed");
        }

        auto output_bytes = out_buffer.size() - out_avail;
        if (output_bytes > 0)
        {
            m_base.write({ reinterpret_cast<char*>(out_buffer.data()), output_bytes });
        }
    }

    return buffer.size();
}

// ---------------------------------------------------------------------

std::streamsize brotli::read(std::span<char> buffer)
{
    auto next_out = buffer.data();
    size_t available_out = buffer.size();

    while (available_out > 0
        && (m_dec_result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT
            || m_dec_result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT))
    {
        if (m_in_available == 0 && m_dec_result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
        {
            m_in_available = m_base.read(m_in_buffer);
            m_in_next = reinterpret_cast<const uint8_t*>(m_in_buffer.data());

            if (m_in_available == 0)
            {
                THROW(util::exception, "unexpected end of data");
            }
        }

        m_dec_result = BrotliDecoderDecompressStream(
            dec(), &m_in_available, &m_in_next, &available_out,
            reinterpret_cast<uint8_t**>(&next_out), nullptr);

        if (m_dec_result == BROTLI_DECODER_RESULT_ERROR)
        {
            THROW(util::exception, "BrotliDecoderDecompressStream failed: " +
                std::string(BrotliDecoderErrorString(BrotliDecoderGetErrorCode(dec()))));
        }
    }

    return buffer.size() - available_out;
}

// ---------------------------------------------------------------------

bool brotli::valid() const
{
    return m_base.valid();
}

// ---------------------------------------------------------------------

void brotli::finish()
{
    const uint8_t* next_in = nullptr;
    size_t available_in = 0;

    std::vector<uint8_t> out_buffer(brotli::BUFFER_SIZE);

    while (BrotliEncoderIsFinished(enc()) == BROTLI_FALSE)
    {
        uint8_t* out_next = out_buffer.data();
        std::size_t out_avail = out_buffer.size();

        if (BrotliEncoderCompressStream(
                    enc(), BROTLI_OPERATION_FINISH, &available_in, &next_in,
                    &out_avail, &out_next, nullptr) == BROTLI_FALSE)
        {
            THROW(util::exception, "BrotliEncoderCompressStream(BROTLI_OPERATION_FINISH) failed");
        }

        auto output_bytes = out_buffer.size() - out_avail;
        if (output_bytes > 0)
        {
            m_base.write({ reinterpret_cast<char*>(out_buffer.data()), output_bytes });
        }
    }
}

// ---------------------------------------------------------------------

} // namespace uh::comp
