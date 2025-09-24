#pragma once

#include <common/telemetry/log.h>
#include <entrypoint/http/stream.h>

namespace uh::cluster::proxy {

/**
 * Copy read data to additional socket.
 */
template <typename OutgoingStream>
class forward_stream : public ep::http::socket_stream {
public:
    /**
     * Create a stream that reads incoming data from `s` and forwards
     * it to the configured downstream socket `to`.
     */
    forward_stream(boost::asio::ip::tcp::socket& s, OutgoingStream& to,
                   std::size_t buffer_size = 4 * MEBI_BYTE)
        : socket_stream(s, buffer_size),
          m_to(to) {}

    coro<void> consume() override {
        if (m_mode == forwarding) {
            co_await boost::asio::async_write(m_to,
                                              boost::asio::buffer(buffer()));
        }

        co_await socket_stream::consume();
    }

    enum mode { forwarding, deleting };

    void set_mode(mode m) { m_mode = m; }

private:
    OutgoingStream& m_to;
    mode m_mode = deleting;
};

} // namespace uh::cluster::proxy
