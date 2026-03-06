/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
