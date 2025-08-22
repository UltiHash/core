#include "forward_stream.h"

#include <common/telemetry/log.h>

namespace uh::cluster::proxy {

forward_stream::forward_stream(boost::asio::ip::tcp::socket& s,
                               boost::asio::ip::tcp::socket& to,
                               std::size_t buffer_size)
    : socket_stream(s, buffer_size),
      m_to(to) {
}

coro<void> forward_stream::consume() {
    if (m_mode == forwarding) {
        LOG_DEBUG() << peer() << " forwarding " << buffer().size() << " bytes to " << m_to.remote_endpoint();
        co_await boost::asio::async_write(m_to, boost::asio::buffer(buffer()));
    }

    co_await socket_stream::consume();
}

void forward_stream::set_mode(mode m) {
    m_mode = m;
}

} // namespace uh::cluster::proxy
