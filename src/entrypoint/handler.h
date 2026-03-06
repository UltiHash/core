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

#include "command_factory.h"
#include "cors/module.h"
#include "http/request_factory.h"
#include "policy/module.h"

namespace uh::cluster::ep {

class handler : public protocol_handler {
public:
    explicit handler(command_factory&& comm_factory,
                     http::request_factory&& factory,
                     std::unique_ptr<policy::module> policy,
                     std::unique_ptr<cors::module> cors);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    command_factory m_command_factory;
    http::request_factory m_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;

    coro<http::response> handle_request(
        const boost::asio::ip::tcp::endpoint& peer,
        http::stream& s,
        const std::string& id);
};

} // end namespace uh::cluster::ep
