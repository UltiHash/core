#pragma once

#include <common/types/common_types.h>
#include <common/utils/common.h>

#include <boost/asio.hpp>

#include <span>
#include <string>
#include <vector>


namespace uh::cluster::ep::http {

class stream
{
public:
    virtual ~stream() = default;

    /**
     * Return a span containing up to `count` bytes starting at the current read
     * position and update read position. Buffer is maintained by `stream` class
     * and remains valid until the next call to `consume()`.
     *
     * @return span pointing to the data. Will be invalidated by next
     *      `consume()`. Will be empty when there is no more data in the buffer.
     *      In that case call `consume()` to free stored data.
     *
     * TODO this interface makes it hard to distinguish between _no more space in
     * buffer_ and _no more data to read_.
     */
    virtual coro<std::span<const char>> read(std::size_t count) = 0;

    /**
     * Return a span starting at the current read position and ending at the
     * first occurence of `delimiter`. If `delimiter` is not included in the current
     * read buffer, returns an empty span.
     */
    virtual coro<std::span<const char>> read_until(std::string_view delimiter) = 0;

    /**
     * Consume data until current read position. Invalidate all previously returned spans.
     * This will free all previously read data.
     */
    virtual coro<void> consume() = 0;

    /**
     * Write to the socket
     */
    virtual coro<std::size_t> write(std::span<const char> buffer) = 0;

    /**
     * Return a string identifying peer.
     */
    virtual std::string peer() const = 0;

    /**
     * Return size of underlying buffer.
     */
    virtual std::size_t buffer_size() const = 0;
};

class socket_stream : public stream
{
public:
    socket_stream(boost::asio::ip::tcp::socket& s,
                  std::size_t buffer_size = 4 * MEBI_BYTE);

    coro<std::span<const char>> read(std::size_t count) override;
    coro<std::span<const char>> read_until(std::string_view delimiter) override;
    coro<std::size_t> write(std::span<const char> buffer) override;

    coro<void> consume() override;
    std::string peer() const override;

    std::size_t buffer_size() const override;

protected:
    /// return current read buffer that is unconsumed
    std::span<const char> buffer() const;

private:
    coro<void> fill();

    boost::asio::ip::tcp::socket& m_s;
    std::size_t m_buffer_size;
    std::vector<char> m_buffer;
    // invariant: `m_get_ptr <= m_put_ptr`
    // invariant: `m_get_ptr <= m_buffer_size`
    // invariant: `m_put_ptr <= m_buffer_size`
    std::size_t m_get_ptr;
    std::size_t m_put_ptr;
};

} // namespace uh::cluster::ep::http
