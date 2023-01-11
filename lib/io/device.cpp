#include "device.h"


namespace uh::io
{

// ---------------------------------------------------------------------

boost_device::boost_device(std::shared_ptr<device> dev)
    : m_dev(dev)
{
}

// ---------------------------------------------------------------------

std::streamsize boost_device::write(const char* s, std::streamsize n)
{
    return m_dev->write(std::span<const char>(s, n));
}

// ---------------------------------------------------------------------

std::streamsize boost_device::read(char*s, std::streamsize n)
{
    return m_dev->read(std::span<char>(s, n));
}

// ---------------------------------------------------------------------

std::vector<char> read_to_buffer(device& dev, std::streamsize chunk_size)
{
    std::streamsize read = 0;
    std::streamsize pos = 0;
    std::vector<char> buffer;

    do
    {
        buffer.resize(buffer.size() + chunk_size);

        read = dev.read({ buffer.begin() + pos, buffer.size() - pos });
        pos += read;
        buffer.resize(pos);
    }
    while (read != 0);

    return buffer;
}

// ---------------------------------------------------------------------

} // namespace uh::io
