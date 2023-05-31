#ifndef IO_DEVICE_GENERATOR_H
#define IO_DEVICE_GENERATOR_H

#include <io/device.h>

#include <memory>


namespace uh::io
{

// ---------------------------------------------------------------------

/**
 * Read data chunks from a device and implament the data_generator interface.
 */
class device_generator : public data_generator
{
public:
    /**
     * @param dev the device to read from
     * @param size the size of the device. No more data than this value will be read.
     * @param chunk_size size of the produced data_chunks
     */
    device_generator(
        std::unique_ptr<io::device> dev,
        std::size_t size,
        std::size_t chunk_size = 16 * 1024);

    std::optional<data_chunk> next() override;

    std::size_t size() const override;

private:
    std::unique_ptr<io::device> m_dev;
    std::size_t m_size;
    std::size_t m_chunk_size;

    std::size_t m_offset;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
