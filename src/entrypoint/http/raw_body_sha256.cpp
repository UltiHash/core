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

#include "raw_body_sha256.h"

#include <common/utils/strings.h>

namespace uh::cluster::ep::http {

raw_body_sha256::raw_body_sha256(stream& s,
                                 raw_request& req, std::string signature)
    : raw_body(s, req),
      m_signature(std::move(signature)) {}

coro<std::span<const char>> raw_body_sha256::read(std::size_t count) {

    auto rv = co_await raw_body::read(count);

    m_hash.consume(rv);

    if (rv.empty()) {
        auto sig = to_hex(m_hash.finalize());
        if (sig != m_signature) {
            throw std::runtime_error("body signature mismatch");
        }
    }

    co_return rv;
}

} // namespace uh::cluster::ep::http
