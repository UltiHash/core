#include "device_generator.h"

#include <util/exception.h>


namespace uh::io
{

// ---------------------------------------------------------------------

device_generator::device_generator(
    std::unique_ptr<io::device> dev,
    std::size_t size,
    std::size_t chunk_size)
    : m_dev(std::move(dev)),
      m_size(size),
      m_chunk_size(chunk_size),
      m_offset(0u)
{
}

// ---------------------------------------------------------------------

std::optional<data_chunk> device_generator::next()
{
    std::size_t count = std::min(m_chunk_size, m_size - m_offset);
    std::vector<char> buffer(count);

    auto read = m_dev->read(buffer);
    if (read < count)
    {
        THROW (util::exception, "not enough data provided by underlying device");
    }

    m_offset += read;
    return buffer;
}

// ---------------------------------------------------------------------

std::size_t device_generator::size() const
{
    return m_size - m_offset;
}

// ---------------------------------------------------------------------

} // namespace uh::io
