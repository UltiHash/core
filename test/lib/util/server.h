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

namespace uh::test {

// ---------------------------------------------------------------------

class server {
public:
    server(const std::string& addr, uint16_t port);

    ~server();

private:
    void accept();
    void run();

    boost::asio::io_context m_ioc;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::thread m_thread;
};

// ---------------------------------------------------------------------

} // namespace uh::test
