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

} // namespace uh::io
