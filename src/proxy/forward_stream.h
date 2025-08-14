#pragma once

#include <entrypoint/http/stream.h>

namespace uh::cluster::proxy {

/**
 * Copy read data to additional socket.
 */
class forward_stream : public ep::http::socket_stream {
public:
    /**
     * Create a stream that reads incoming data from `s` and forwards
     * it to the configured downstream socket `to`.
     */
    forward_stream(boost::asio::ip::tcp::socket& s,
                   boost::asio::ip::tcp::socket& to,
                   std::size_t buffer_size = 4 * MEBI_BYTE);

    coro<void> consume() override;

    enum mode { forwarding, deleting };

    void set_mode(mode m);

private:
    boost::asio::ip::tcp::socket& m_to;
    mode m_mode = deleting;
};

} // namespace uh::cluster::proxy
