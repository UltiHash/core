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

#include "server.h"

using namespace boost::asio;

namespace uh::test {

namespace {

ip::tcp::acceptor acceptor(io_context& ioc, const std::string& addr,
                           uint16_t port) {
    ip::tcp::acceptor acceptor(ioc);

    acceptor.open(ip::tcp::v4());
    acceptor.set_option(socket_base::reuse_address(true));
    acceptor.bind(ip::tcp::endpoint{ip::make_address(addr), port});
    acceptor.listen(16);

    return acceptor;
}

} // namespace

server::server(const std::string& addr, uint16_t port)
    : m_ioc(),
      m_acceptor(acceptor(m_ioc, addr, port)),
      m_thread([this]() { run(); }) {}

server::~server() {
    m_ioc.stop();
    m_thread.join();
}

void server::accept() {
    auto sock = std::make_shared<ip::tcp::socket>(m_ioc);
    m_acceptor.async_accept(*sock, [this, sock](auto err) { accept(); });
}

void server::run() {
    accept();
    m_ioc.run();
}

} // namespace uh::test
