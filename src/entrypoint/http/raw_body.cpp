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

#include "raw_body.h"

using namespace boost;

namespace uh::cluster::ep::http {

namespace {

std::size_t get_length(raw_request& req) {

    if (auto content_length = req.optional("content-length"); content_length) {
        return std::stoul(*content_length);
    }

    return 0ul;
}

} // namespace

raw_body::raw_body(stream& s, raw_request& req)
    : m_s(s),
      m_length(get_length(req)) {}

std::optional<std::size_t> raw_body::length() const { return m_length; }

coro<std::span<const char>> raw_body::read(std::size_t count) {
    auto rv = co_await m_s.read(std::min(count, m_length));

    m_length -= rv.size();

    co_return rv;
}

coro<void> raw_body::consume() {
    co_await m_s.consume();
}

std::size_t raw_body::buffer_size() const {
    return m_s.buffer_size();
}

} // namespace uh::cluster::ep::http
