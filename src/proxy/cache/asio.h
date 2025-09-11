#pragma once

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <common/types/common_types.h>
#include <concepts>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>

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

} // namespace uh::cluster::proxy::cache
