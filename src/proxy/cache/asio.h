#pragma once

#include <boost/beast/core/buffers_to_string.hpp>
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
    { r.put(sv) } -> std::same_as<coro<void>>;
};

template <typename T>
concept WriterBodyType = requires(T w, std::span<char> sv) {
    { w.get(sv) } -> std::same_as<coro<std::span<const char>>>;
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
        co_await reader.put(sv);
        co_await s.consume();
        size -= sv.size();
    }
}

/*
 * It consumes automatically
 *
 * TODO: use socket instead of stream
 * TODO: Choose which namespace we will use
 */
template <std::size_t buffer_size, typename T>
coro<void> async_write(ep::http::stream& s, T& t) {
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

    char _buf[2][buffer_size];
    char* rbuf = _buf[0];
    char* wbuf = _buf[1];

    for (auto data = co_await writer.get({rbuf, buffer_size}); !data.empty();) {
        std::swap(rbuf, wbuf);
        auto [d, _] =
            co_await (writer.get({rbuf, buffer_size}) && s.write(data));
        data = d;
    }
}

template <typename Awaitable> coro<void> ignore_need_buffer(Awaitable&& op) {
    boost::system::error_code ec;
    co_await op(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec && ec != boost::beast::http::error::need_buffer) {
        throw boost::system::system_error(ec);
    }
}

template <class Message> std::string serialize_header(Message& msg) {
    using body_type = typename std::decay_t<Message>::body_type;
    using fields_type = typename std::decay_t<Message>::fields_type;
    constexpr bool is_request = std::decay_t<Message>::is_request::value;
    boost::beast::http::serializer<is_request, body_type, fields_type> sr{msg};
    sr.split(true);
    std::string header_str;
    while (!sr.is_done()) {
        auto const buf = sr.get();
        // buffers_to_string handles any buffer sequence or single buffer
        header_str += boost::beast::buffers_to_string(buf);
        sr.consume(boost::asio::buffer_size(buf));
    }
    return header_str;
}

template <typename ServerSocketType, typename Serializer, typename SyncType>
coro<std::size_t> async_write_store_header(ServerSocketType& server_socket,
                                           Serializer& sr, SyncType& sync) {
    std::ostringstream oss;
    boost::system::error_code ec;
    sr.split(true);
    write_ostream(oss, sr, ec);
    auto header_str = oss.str();
    if (header_str.size() == 0) {
        throw std::runtime_error("Could not serialize header");
    }
    co_await (sync.put(header_str) && [&]() -> coro<void> {
        co_await async_write(server_socket, boost::asio::buffer(header_str));
    }());
    sync.set_header_size(header_str.size());
    co_return header_str.size();
}

template <std::size_t buffer_size, class AsyncReadStream,
          class AsyncWriteStream, class DynamicBuffer, class Parser,
          class Serializer>
coro<std::size_t>
async_relay_body(AsyncReadStream& input, AsyncWriteStream& output,
                 DynamicBuffer& buffer, Parser& p, Serializer& sr) {
    static_assert(boost::beast::is_async_write_stream<AsyncWriteStream>::value,
                  "AsyncWriteStream requirements not met");
    static_assert(boost::beast::is_async_read_stream<AsyncReadStream>::value,
                  "AsyncReadStream requirements not met");

    char _buf[2][buffer_size];
    char* rbuf = _buf[0];
    char* wbuf = _buf[1];

    auto read = [&](std::span<char> sv) -> coro<std::size_t> {
        p.get().body().rdata = sv.data();
        p.get().body().rsize = sv.size();
        co_await ignore_need_buffer(
            [&](auto token) { return async_read(input, buffer, p, token); });

        co_return sv.size() - p.get().body().rsize;
    };

    auto write = [&](std::span<const char> sv) -> coro<void> {
        p.get().body().more = sv.size() != 0;
        p.get().body().wdata = sv.data();
        p.get().body().wsize = sv.size();
        co_await ignore_need_buffer(
            [&](auto token) { return async_write(output, sr, token); });
    };

    std::size_t total_bytes = 0;
    for (auto bytes_read = co_await read({rbuf, buffer_size});
         !p.is_done() || !sr.is_done();) {
        std::swap(rbuf, wbuf);
        bytes_read =
            co_await (read({rbuf, buffer_size}) && write({wbuf, bytes_read}));
        total_bytes += bytes_read;
    }
    co_return total_bytes;
}

template <std::size_t buffer_size, class AsyncReadStream,
          class AsyncWriteStream, class DynamicBuffer, class Parser,
          class Serializer, class PayloadSync>
coro<std::size_t> async_relay_store_body(AsyncReadStream& input,
                                         AsyncWriteStream& output,
                                         DynamicBuffer& buffer, Parser& p,
                                         Serializer& sr, PayloadSync& sync) {
    static_assert(boost::beast::is_async_write_stream<AsyncWriteStream>::value,
                  "AsyncWriteStream requirements not met");
    static_assert(boost::beast::is_async_read_stream<AsyncReadStream>::value,
                  "AsyncReadStream requirements not met");

    char _buf[2][buffer_size];
    char* rbuf = _buf[0];
    char* wbuf = _buf[1];

    auto read = [&](std::span<char> sv) -> coro<std::size_t> {
        p.get().body().rdata = sv.data();
        p.get().body().rsize = sv.size();
        co_await ignore_need_buffer(
            [&](auto token) { return async_read(input, buffer, p, token); });

        co_return sv.size() - p.get().body().rsize;
    };

    auto write = [&](std::span<const char> sv) -> coro<void> {
        p.get().body().more = sv.size() != 0;
        p.get().body().wdata = sv.data();
        p.get().body().wsize = sv.size();
        co_await ignore_need_buffer(
            [&](auto token) { return async_write(output, sr, token); });
    };

    std::size_t total_bytes = 0;
    for (auto bytes_read = co_await read({rbuf, buffer_size});
         !p.is_done() || !sr.is_done();) {
        std::swap(rbuf, wbuf);
        bytes_read = co_await (
            (read({rbuf, buffer_size}) && write({wbuf, bytes_read})) &&
            sync.put({wbuf, bytes_read}));
        total_bytes += bytes_read;
    }
    co_return total_bytes;
}

} // namespace uh::cluster::proxy::cache
namespace boost::beast::http {
// The detail namespace means "not public"
namespace detail {

// This helper is needed for C++11.
// When invoked with a buffer sequence, writes the buffers `to the
// std::ostream`.
template <class Serializer> class write_ostream_helper {
    Serializer& sr_;
    std::ostream& os_;

public:
    write_ostream_helper(Serializer& sr, std::ostream& os)
        : sr_(sr),
          os_(os) {}

    // This function is called by the serializer
    template <class ConstBufferSequence>
    void operator()(error_code& ec, ConstBufferSequence const& buffers) const {
        // Error codes must be cleared on success
        ec = {};

        // Keep a running total of how much we wrote
        std::size_t bytes_transferred = 0;

        // Loop over the buffer sequence
        for (auto it = boost::asio::buffer_sequence_begin(buffers);
             it != boost::asio::buffer_sequence_end(buffers); ++it) {
            // This is the next buffer in the sequence
            boost::asio::const_buffer const buffer = *it;

            // Write it to the std::ostream
            os_.write(reinterpret_cast<char const*>(buffer.data()),
                      buffer.size());

            // If the std::ostream fails, convert it to an error code
            if (os_.fail()) {
                ec = make_error_code(errc::io_error);
                return;
            }

            // Adjust our running total
            bytes_transferred += buffer_size(buffer);
        }

        // Inform the serializer of the amount we consumed
        sr_.consume(bytes_transferred);
    }
};

} // namespace detail

/** Write a message to a `std::ostream`.

    This function writes the serialized representation of the
    HTTP/1 message to the sream.

    @param os The `std::ostream` to write to.

    @param msg The message to serialize.

    @param ec Set to the error, if any occurred.
*/
template <typename Serializer>
void write_ostream(std::ostream& os, Serializer& sr, error_code& ec) {

    // This lambda is used as the "visit" function
    detail::write_ostream_helper<decltype(sr)> lambda{sr, os};
    do {
        // In C++14 we could use a generic lambda but since we want
        // to require only C++11, the lambda is written out by hand.
        // This function call retrieves the next serialized buffers.
        sr.next(ec, lambda);
        if (ec)
            return;
    } while (!sr.is_done());
}

} // namespace boost::beast::http
