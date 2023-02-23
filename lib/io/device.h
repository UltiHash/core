#ifndef IO_DEVICE_H
#define IO_DEVICE_H

#include <boost/iostreams/categories.hpp>

#include <ios>
#include <memory>
#include <span>
#include <vector>


namespace uh::io
{

// ---------------------------------------------------------------------

/**
 * Generic I/O abstraction class.
 */
class device
{
public:
    virtual ~device() = default;

    /**
     * Write the contents of the span and return the number of bytes written.
     *
     * @throw writing fails for any reason
     */
    virtual std::streamsize write(std::span<const char> buffer) = 0;

    /**
     * Read from the device and store it in the buffer. Return the number of
     * bytes read.
     *
     * @throw error while reading
     */
    virtual std::streamsize read(std::span<char> buffer) = 0;

    /**
     * Return whether this device still can be used.
     */
    virtual bool valid() const = 0;
};

// ---------------------------------------------------------------------

/**
 * Adapter enabling boost iostreams API for devices.
 */
class boost_device
{
public:
    typedef char char_type;
    typedef boost::iostreams::bidirectional_device_tag category;

    boost_device(std::shared_ptr<device> dev);

    std::streamsize write(const char* s, std::streamsize n);
    std::streamsize read(char*s, std::streamsize n);

private:
    std::shared_ptr<device> m_dev;
};

// ---------------------------------------------------------------------

static constexpr std::streamsize DEFAULT_CHUNK_SIZE = 16 * 1024 * 1024;

// ---------------------------------------------------------------------

/**
 * Read the complete device into memory and return it in a vector. `chunk_size`
 * defines how many bytes are read at a time.
 */
std::vector<char> read_to_buffer(
    device& dev,
    std::streamsize chunk_size = DEFAULT_CHUNK_SIZE);

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, device& d);

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
