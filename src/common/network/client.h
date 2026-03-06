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

#include "common/utils/common.h"
#include "common/utils/pool.h"
#include "messenger.h"
#include "tools.h"
#include <boost/asio.hpp>

namespace uh::cluster {

class client {
public:
    using acquired_messenger = pool<messenger>::handle;

    client(boost::asio::io_context& ioc, const std::string& address,
           const std::uint16_t port, const int connections)
        : m_messenger_factory{[&ioc, address, port]() {
              auto endpoint = resolve(address, port).back();
              return std::make_unique<messenger>(
                  ioc, endpoint.address().to_string(), port,
                  messenger::origin::DOWNSTREAM);
          }},
          m_pool([this]() { return m_messenger_factory(); }, connections) {}

    client(client&&) = default;

    coro<acquired_messenger> acquire_messenger() {
        auto ret = co_await m_pool.get();
        if (!ret->connected()) {
            LOG_DEBUG()
                << "acquired messenger is not connected, reconnecting...";
            ret.replace_resource(m_messenger_factory());
        }
        co_return ret;
    }

private:
    std::function<std::unique_ptr<messenger>()> m_messenger_factory;
    pool<messenger> m_pool;
};

} // end namespace uh::cluster
