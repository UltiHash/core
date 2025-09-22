#pragma once

#include <proxy/asio.h>

#include <common/telemetry/trace/awaitable_operators.h>

#include <boost/asio/buffer.hpp>

namespace uh::cluster::proxy::cache::disk {

template <std::size_t buffer_size, typename SocketType, typename BodyType>
coro<void> async_write(SocketType& s, BodyType& reader) {
    using boost::asio::experimental::awaitable_operators::operator&&;

    char _buf[2][buffer_size];
    char* rbuf = _buf[0];
    char* wbuf = _buf[1];

    for (auto data = co_await reader.get({rbuf, buffer_size}); !data.empty();) {
        std::swap(rbuf, wbuf);
        auto d =
            co_await (reader.get({rbuf, buffer_size}) && [&]() -> coro<void> {
                co_await async_write(s, boost::asio::const_buffer(data));
            }());
        data = d;
    }
}

template <typename ServerSocketType, typename Serializer, typename SyncType>
coro<std::size_t> async_write_store_header(ServerSocketType& server_socket,
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

template <std::size_t chunk_size, typename Incomming, typename Outgoing,
          typename PayloadWriter>
coro<void> async_relay_store_body(Incomming& in, Outgoing& out,
                                  boost::beast::flat_buffer& b,
                                  PayloadWriter& writer,
                                  std::size_t payload_size) {
    using boost::asio::experimental::awaitable_operators::operator&&;

    if (payload_size > chunk_size) {
        boost::beast::flat_buffer b2;
        auto* rbuf = &b;
        auto* wbuf = &b2;

        for (auto n = co_await async_read(
                 in, rbuf->prepare(std::min(payload_size, chunk_size) -
                                   rbuf->data().size()));
             n != 0;) {
            std::swap(rbuf, wbuf);
            wbuf->commit(n + wbuf->data().size());
            payload_size -= wbuf->data().size();
            auto new_n = co_await ( //
                [&]() -> coro<std::size_t> {
                    co_return co_await async_read(
                        in, rbuf->prepare(std::min(payload_size, chunk_size)));
                }() && ([&]() -> coro<void> {
                             co_await async_write(out, get_span(wbuf->data()));
                         }() && writer.put(get_span(wbuf->data())) //
                        )                                          //
            );
            wbuf->consume(wbuf->data().size());
            n = new_n;
        }
    } else {
        auto n =
            co_await async_read(in, b.prepare(payload_size - b.data().size()));
        b.commit(n + b.data().size());
        co_await ([&]() -> coro<void> {
            co_await async_write(out, get_span(b.data()));
        }() && writer.put(get_span(b.data())));
        b.consume(b.data().size());
    }

    b.shrink_to_fit();
}

} // namespace uh::cluster::proxy::cache::disk
