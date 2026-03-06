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

#include "string_body.h"

namespace uh::cluster::ep::http {

string_body::string_body(std::string&& body)
    : m_body(std::move(body)),
      m_read(0ull) {}

std::optional<std::size_t> string_body::length() const { return m_body.size(); }

coro<std::span<const char>> string_body::read(std::size_t count) {
    auto len = std::min(count, m_body.size() - m_read);

    auto rv = std::span{ &m_body[m_read], len};
    m_read += len;

    co_return rv;
}

coro<void> string_body::consume() {
    co_return;
}

std::size_t string_body::buffer_size() const {
    return m_body.size();
}

} // namespace uh::cluster::ep::http
