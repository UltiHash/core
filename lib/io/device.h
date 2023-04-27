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

static constexpr std::streamsize IO_DEFAULT_CHUNK_SIZE = 16 * 1024 * 1024;

// ---------------------------------------------------------------------

/**
 * Use this class to construct an "owning wrapper" around a device. Usually
 * device wrapper expect the wrapped device as reference. With `device_wrapper`
 * you can make tie the lifetime of the wrapped device to its wrapper:
 */
template <typename wrapper, typename ... args>
class device_wrapper : public io::device
{
public:
    device_wrapper(std::unique_ptr<device>&& base, args... a)
        : m_base(std::move(base)),
          m_wrapper(*m_base, std::forward(a)...)
    {
    }

    std::streamsize write(std::span<const char> buffer) override
    {
        return m_wrapper.write(buffer);
    }

    std::streamsize read(std::span<char> buffer) override
    {
        return m_wrapper.read(buffer);
    }

    bool valid() const override
    {
        return m_wrapper.valid();
    }

private:
    std::unique_ptr<device> m_base;
    wrapper m_wrapper;
};

// ---------------------------------------------------------------------

template <typename wrapper, typename ... args>
std::unique_ptr<io::device> owning_wrapper(std::unique_ptr<device>&& base, args... a)
{
    return std::make_unique<device_wrapper<wrapper, args...>>(std::move(base), a...);
}

// ---------------------------------------------------------------------

/**
 * Read the complete device into memory and return it in a vector. `chunk_size`
 * defines how many bytes are read at a time.
 */
std::vector<char> read_to_buffer(
    device& dev,
    std::streamsize chunk_size = IO_DEFAULT_CHUNK_SIZE);

// ---------------------------------------------------------------------

std::size_t write_from_buffer(
    device& dev,
    std::span<char> buffer);

// ---------------------------------------------------------------------

/**
 * Copy the complete device to the given std::ostream&. Return number of
 * bytes written.
 */
std::size_t copy(device& d, std::ostream& out);

// ---------------------------------------------------------------------

/**
 * Copy the complete device `in` to the device `out`. Return number of
 * bytes written.
 */
std::size_t copy(device& in, device& out);

// ---------------------------------------------------------------------

/**
 * Write the buffer to the device. This function guarantees that the complete
 * buffer is written by calling `dev.write()` repeatedly until everything is
 * written.
 *
 * @throws when `dev.write()` throws
 * @return number of bytes written
 */
std::streamsize write(io::device& dev, std::span<const char> buffer);

// ---------------------------------------------------------------------

/**
 * Read from the device to the buffer. This function guarantees that all bytes
 * available on the device up to `buffer.size()` are read by calling `dev.read()`
 * repeatedly until `buffer` is filled or `dev.read()` signales end of data.
 *
 * @throws when `dev.read()` throws
 * @return number of bytes read
 */
std::streamsize read(io::device& dev, std::span<char> buffer);

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
