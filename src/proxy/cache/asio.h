#pragma once

#include <boost/beast/http.hpp>
#include <common/types/common_types.h>
#include <concepts>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>
#include <proxy/cache/awaitable_operators.h>
#include <proxy/cache/double_buffer_body.h>

using namespace boost::asio::experimental::awaitable_operators;

namespace uh::cluster::proxy::cache {

template <typename T>
concept ReaderBodyType = requires(T r, std::span<const char> sv) {
    { r.put(sv) } -> std::same_as<coro<std::size_t>>;
};

template <typename T>
concept WriterBodyType = requires(T w) {
    { w.get() } -> std::same_as<coro<std::span<const char>>>;
};

template <typename T>
concept BodyType = requires {
    typename T::writer;
    typename T::reader;
    requires WriterBodyType<typename T::writer>;
    requires ReaderBodyType<typename T::reader>;
};

template <BodyType Body> typename Body::reader make_reader(Body& b) {
    return typename Body::reader(b);
}

template <BodyType Body> typename Body::writer make_writer(Body& b) {
    return typename Body::writer(b);
}

/*
 * async_read gets stream, body and size for it's input.
 *
 * size can be replaced with parser implementation
 */
template <typename S, typename T>
requires std::is_base_of_v<ep::http::stream, S>
coro<void> async_read(S& s, T& t, std::size_t size) {
    auto&& reader = [&]() -> auto&& {
        if constexpr (BodyType<T>) {
            return make_reader(t);
        } else if constexpr (ReaderBodyType<T>) {
            return t;
        } else {
            static_assert(BodyType<T> || ReaderBodyType<T>,
                          "T must satisfy BodyType or ReaderBodyType");
        }
    }();

    while (size > 0) {
        auto sv = co_await s.read(size);
        if (sv.empty())
            break;
        auto read = co_await reader.put(sv);
        if (read != sv.size()) {
            throw std::runtime_error(
                "reader_body put() returned unexpected size");
        }
        co_await s.consume();
        size -= sv.size();
    }
}

/*
 * It consumes automatically
 */
template <typename T> coro<void> async_write(ep::http::stream& s, T& t) {
    auto&& writer = [&]() -> auto&& {
        if constexpr (BodyType<T>) {
            return make_writer(t);
        } else if constexpr (WriterBodyType<T>) {
            return t;
        } else {
            static_assert(BodyType<T> || WriterBodyType<T>,
                          "T must satisfy BodyType or WriterBodyType");
        }
    }();

    if constexpr (T::support_double_buffer::value) {
        for (auto data = co_await writer.get(); !data.empty();) {
            auto [d, _] = co_await (writer.get() && s.write(data));
            data = d;
        }
    } else {
        while (true) {
            auto data = co_await writer.get();
            if (data.empty())
                break;
            co_await s.write(data);
        }
    }
}

template <typename Awaitable> coro<void> ignore_need_buffer(Awaitable&& op) {
    boost::system::error_code ec;
    co_await op(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec && ec != boost::beast::http::error::need_buffer) {
        throw boost::system::system_error(ec);
    }
}

template <bool isRequest, class AsyncWriteStream, class AsyncReadStream,
          class DynamicBuffer, class Parser, class Serializer>
coro<void> relay(AsyncWriteStream& output, AsyncReadStream& input,
                 DynamicBuffer& buffer, Parser& p, Serializer& sr) {
    static_assert(boost::beast::is_async_write_stream<AsyncWriteStream>::value,
                  "AsyncWriteStream requirements not met");
    static_assert(boost::beast::is_async_read_stream<AsyncReadStream>::value,
                  "AsyncReadStream requirements not met");

    constexpr std::size_t buf_size = 2_KiB;
    char _buf[2][buf_size];
    char* rbuf = _buf[0];
    char* wbuf = _buf[1];

    auto read = [&](char* buf) -> coro<std::size_t> {
        p.get().body().rdata = buf;
        p.get().body().rsize = buf_size;
        co_await ignore_need_buffer(
            [&](auto token) { return async_read(input, buffer, p, token); });

        co_return buf_size - p.get().body().rsize;
    };

    auto write = [&](char* buf, std::size_t size) -> coro<void> {
        p.get().body().more = size != 0;
        p.get().body().wdata = buf;
        p.get().body().wsize = size;
        co_await ignore_need_buffer(
            [&](auto token) { return async_write(output, sr, token); });
    };

    for (auto bytes_read = co_await read(rbuf);
         !p.is_done() || !sr.is_done();) {
        std::swap(rbuf, wbuf);
        bytes_read = co_await (read(rbuf) && write(wbuf, bytes_read));
    }
}

} // namespace uh::cluster::proxy::cache
