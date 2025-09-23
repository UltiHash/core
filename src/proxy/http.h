#pragma once

#include <common/telemetry/trace/awaitable_operators.h>
#include <common/types/common_types.h>

#include <proxy/asio.h>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include <concepts>
#include <span>

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

template <typename Message>
std::optional<std::uint64_t> get_content_length(const Message& msg) {
    auto it = msg.find(boost::beast::http::field::content_length);
    if (it != msg.end()) {
        try {
            return std::stoull(
                std::string(it->value().data(), it->value().size()));
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace boost::beast::http

namespace uh::cluster::proxy {

template <typename SourceType, typename Parser>
coro<void> async_read_header(const SourceType& source, Parser& parser) {
    auto header_size = std::vector<char>(source->get_header_size());
    auto header = co_await source->get(header_size);

    parser.body_limit(std::numeric_limits<std::uint64_t>::max());
    boost::system::error_code ec;
    parser.put(boost::asio::const_buffer(header), ec);
    if (ec) {
        throw boost::system::system_error(ec);
    }
}

template <typename ServerSocketType, typename Serializer, typename SyncType>
coro<std::size_t> async_write_header(ServerSocketType& server_socket,
                                     Serializer& sr, SyncType& sync) {
    using boost::asio::experimental::awaitable_operators::operator&&;
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

template <std::size_t chunk_size, typename Incomming, typename SyncType>
coro<void> async_read(
    Incomming& in, boost::beast::flat_buffer& b, std::size_t payload_size,
    SyncType&& sync,
    std::function<coro<void>()> precursor = []() -> coro<void> { co_return; }) {
    using boost::asio::experimental::awaitable_operators::operator&&;

    if (b.data().size() >= payload_size) {
        auto sv = std::span<const char>(
            static_cast<const char*>(b.data().data()), payload_size);
        co_await (sync.put(sv) && precursor());
        b.consume(sv.size());

    } else {
        auto read = [&](auto& s, auto& buffer,
                        std::size_t required) -> coro<std::size_t> {
            co_return co_await async_read(s, buffer.prepare(required));
        };
        if (payload_size > chunk_size) {
            boost::beast::flat_buffer b2(chunk_size);
            auto* rbuf = &b;
            auto* wbuf = &b2;

            for (auto n = co_await (read(in, *rbuf,
                                         std::min(payload_size, chunk_size) -
                                             rbuf->data().size()) &&
                                    precursor());
                 n != 0;) {
                std::swap(rbuf, wbuf);
                wbuf->commit(n);
                if (wbuf->data().size() != std::min(payload_size, chunk_size)) {
                    throw std::runtime_error("buffer size mismatch");
                }
                payload_size -= wbuf->data().size();
                auto new_n = co_await (
                    read(in, *rbuf, std::min(payload_size, chunk_size)) &&
                    sync.put(get_span(wbuf->data())));

                wbuf->consume(wbuf->data().size());
                n = new_n;
            }
        } else {
            auto n = co_await (read(in, b, payload_size - b.data().size()) &&
                               precursor());
            b.commit(n);
            co_await sync.put(get_span(b.data()));
            b.consume(b.data().size());
        }
    }

    b.shrink_to_fit();
}

template <std::size_t buffer_size, typename SocketType, typename SourceType>
coro<void> async_write(
    SocketType& s, SourceType& source,
    std::function<coro<void>()> precursor = []() -> coro<void> { co_return; }) {
    using boost::asio::experimental::awaitable_operators::operator&&;

    char _buf[2][buffer_size];
    char* rbuf = _buf[0];
    char* wbuf = _buf[1];

    for (auto data = co_await (source.get({rbuf, buffer_size}) && precursor());
         !data.empty();) {
        std::swap(rbuf, wbuf);
        auto d =
            co_await (source.get({rbuf, buffer_size}) && [&]() -> coro<void> {
                co_await async_write(s, boost::asio::const_buffer(data));
            }());
        data = d;
    }
}

} // namespace uh::cluster::proxy
