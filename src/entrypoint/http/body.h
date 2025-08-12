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
     * Fill the provided span with the next bytes of the body, return number of
     * bytes written.
     */
    virtual coro<std::size_t> read(std::span<char>) = 0;

    /*
     * Return a raw buffer from the previous read call.
     */
    virtual std::vector<boost::asio::const_buffer>
    get_raw_buffer() const = 0;
};

} // namespace uh::cluster::ep::http
