#ifndef CHUNKING_BUFFER_H
#define CHUNKING_BUFFER_H

#include <io/device.h>

#include <span>
#include <vector>


namespace uh::chunking
{

// ---------------------------------------------------------------------

/**
 * Offers a buffer on top of io::device that guarantees to hold `size`
 * bytes in consecutive memory.
 */
class buffer
{
public:
    /**
     * Create a buffer for `dev` with guaranteed size.
     */
    buffer(io::device& dev, std::size_t size);

    /**
     * Fill the buffer from the underlying device and return number of bytes in the
     * buffer. This will invalidate all marks and returned spans.
     *
     * @throw io::device::read throws for underlying device
     * @return number of bytes in buffer
     */
    std::size_t fill_buffer();

    /**
     * Read the next byte from the buffer.
     */
    int next_byte();

    /**
     * Read the whole buffer.
     */
    char* next_buffer();

    /**
     * Skip over the next `count` bytes.
     */
    void skip(std::size_t count);

    std::size_t length() const;

    class pos
    {
    private:
        friend buffer;

        pos(std::size_t start);

        std::size_t m_start = 0;
    };

    /**
     * Get a pointer to the current position in the buffer. This pointer
     * can be used to retrieve the previously skipped memory of the buffer.
     * It is invalidated by calls to `fill_buffer`.
     */
    pos mark() const;

    /**
     * Return the number of bytes from a marked position.
     */
    std::size_t length(const pos& p) const;

    /**
     * Return consecutive memory starting at the location pointed to by `p`.
     */
    std::span<char> data(const pos& p);

    /**
     * Return the complete contents of the buffer.
     */
    std::span<char> data();

private:
    io::device& m_in;
    std::vector<char> m_buffer;

    std::size_t m_wptr;
    std::size_t m_rptr;
    std::size_t m_size;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
