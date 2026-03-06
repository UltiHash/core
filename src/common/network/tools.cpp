// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tools.h"

#include <boost/asio.hpp>

namespace asio = boost::asio;

namespace uh::cluster {

std::list<asio::ip::tcp::endpoint> resolve(const std::string& address,
                                           uint16_t port) {
    asio::io_context io_service;
    asio::ip::tcp::resolver resolver(io_service);

    auto results = resolver.resolve(address, std::to_string(port));
    return std::list<asio::ip::tcp::endpoint>(results.cbegin(), results.cend());
}

} // namespace uh::cluster
