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

#include <common/telemetry/trace/awaitable_operators.h>
#include <common/types/common_types.h>

namespace uh::cluster::proxy {

template <typename T, typename U> class tee {
public:
    tee(T& t, U& u)
        : m_t{t},
          m_u{u} {}

    coro<void> put(std::span<const char> sv) {
        using boost::asio::experimental::awaitable_operators::operator&&;

        if (sv.size() == 0) {
            co_return;
        }
        co_await (m_t.put(sv) && m_u.put(sv));
    }

private:
    T& m_t;
    U& m_u;
};

} // namespace uh::cluster::proxy
