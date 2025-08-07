#pragma once

#include <entrypoint/commands/command.h>
#include <entrypoint/directory.h>
#include <entrypoint/http/raw_response.h>

namespace uh::cluster {

inline coro<ep::http::response>
forward(ep::http::request& req, boost::asio::ip::tcp::socket& endpoint) {

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

    auto rawresp = co_await ep::http::raw_response::read(endpoint);

    // if (rawresp.optional("Transfer-Encoding").value_or("") == "chunked") {
    //     auto body = std::make_unique<chunked_body>(endpoint, rawresp);
    //     co_return std::make_unique<request>(std::move(rawresp),
    //     std::move(body),
    //                                         std::move(user));
    // } else {
    //     auto body = std::make_unique<raw_body>(endpoint, rawresp);
    //     co_return std::make_unique<request>(std::move(rawresp),
    //     std::move(body),
    //                                         std::move(user));
    // }
    co_return ep::http::response(ep::http::status::no_content);
}

} // namespace uh::cluster
