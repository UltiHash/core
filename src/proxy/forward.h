#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>

namespace uh::cluster {

inline coro<void> forward(ep::http::request& req,
                          boost::asio::ip::tcp::socket& endpoint) {

    constexpr std::size_t buffer_size = 64 * KIBI_BYTE;
    std::string buffer;
    buffer.resize(buffer_size);
    std::size_t count = 0;

    co_await boost::asio::async_write(endpoint,
                                      req.get_header().get_raw_buffer());
    do {
        count = co_await req.read_body({&buffer[0], buffer_size});
        auto raw_buffer = req.get_raw_buffer();
        co_await boost::asio::async_write(endpoint, raw_buffer);
    } while (count == buffer_size);
}

} // namespace uh::cluster
