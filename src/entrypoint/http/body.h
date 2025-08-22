#pragma once

#include <common/types/common_types.h>
#include <span>

namespace uh::cluster::ep::http {

class body {
public:
    virtual ~body() = default;

    /**
     * Return remaining length of body if available.
     */
    virtual std::optional<std::size_t> length() const = 0;

    /**
     * Read next chunk of data. May return less then `count` bytes.
     * Return zero-length span on end of body.
     */
    virtual coro<std::span<const char>> read(std::size_t count) = 0;

    /**
     * Clear underlying stream buffers
     */
    virtual coro<void> consume() = 0;

    virtual std::size_t buffer_size() const = 0;
};

template <typename container = std::string>
coro<container> copy_to_buffer(body& s) {
    std::size_t bs = s.buffer_size();

    container rv;
    std::size_t len = 0;

    std::span<const char> data = co_await s.read(bs);

    while (!data.empty()) {
        rv.resize(len + data.size());
        memcpy(&rv[len], data.data(), data.size());
        len += data.size();

        co_await s.consume();
        data = co_await s.read(bs);
    }

    co_await s.consume();
    co_return std::move(rv);
}

} // namespace uh::cluster::ep::http
