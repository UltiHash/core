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

#include <boost/asio.hpp>
#include <memory>

namespace uh::cluster {

template <typename service_interface> struct service_factory {
public:
    service_factory(boost::asio::io_context& ioc, int connections)
        : m_ioc(ioc),
          m_connections(connections) {}

    std::shared_ptr<service_interface> make_service(const std::string& hostname,
                                                    uint16_t port) {
        return make_remote_service(hostname, port);
    }

private:
    std::shared_ptr<service_interface>
    make_remote_service(const std::string& hostname, uint16_t port);

    boost::asio::io_context& m_ioc;
    int m_connections;
};

} // namespace uh::cluster
