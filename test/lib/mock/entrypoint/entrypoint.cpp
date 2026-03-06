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

#include "entrypoint.h"

using namespace uh::cluster::ep::http;
using namespace uh::cluster::ep::policy;
using namespace uh::cluster::ep::user;

namespace uh::cluster::test {

mock_command::mock_command(const std::string& id)
    : m_id(id) {}

coro<response> mock_command::handle(request&) { co_return response{}; }

coro<void> mock_command::validate(const request& req) { co_return; }

std::string mock_command::action_id() const { return m_id; }

coro<std::span<const char>> mock_body::read(std::size_t len) { co_return std::span<const char>{}; }

std::optional<std::size_t> mock_body::length() const { return {}; }

coro<void> mock_body::consume() {
    co_return;
}

std::size_t mock_body::buffer_size() const {
    return 0ull;
}

ep::http::request make_request(const std::string& code,
                               const std::string& principal) {
    boost::beast::http::request_parser<boost::beast::http::empty_body> parser;
    boost::beast::error_code ec;

    parser.put(boost::asio::buffer(code), ec);

    return request(raw_request::from_string(parser.release(), {}),
                   std::make_unique<mock_body>(), user{.arn = principal});
}

variables vars(std::initializer_list<std::pair<std::string, std::string>> v) {
    static request req;
    static mock_command cmd;
    variables rv(req, cmd);

    for (auto& p : v) {
        rv.put(std::move(p.first), std::move(p.second));
    }

    return rv;
}

} // namespace uh::cluster::test
