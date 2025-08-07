#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>

namespace uh::cluster {

inline coro<ep::http::response>
forward(ep::http::request& req, boost::asio::ip::tcp::socket& endpoint) {

    constexpr std::size_t buffer_size = 64 * KIBI_BYTE;
    std::string buffer;
    buffer.resize(buffer_size);
    std::size_t count = 0;

    do {
        count = co_await req.read_body({&buffer[0], buffer_size});
        co_await boost::asio::async_write(
            endpoint, boost::asio::buffer(buffer.data(), count));
    } while (count == buffer_size);

    co_return ep::http::response(ep::http::status::no_content);
}

} // namespace uh::cluster
