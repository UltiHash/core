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

#include "address.h"
#include "scoped_buffer.h"

#include <common/telemetry/trace/trace_asio.h>

#include <boost/asio/awaitable.hpp>
#include <chrono>

namespace uh::cluster {

struct dedupe_response {
    std::size_t effective_size{};
    address addr;

    void append(const dedupe_response& other) {
        effective_size += other.effective_size;
        addr.append(other.addr);
    }
};

struct allocation_t {
    std::size_t offset;
    std::size_t size;
};

struct refcount_t {
    std::size_t stripe_id;
    std::size_t count;

    bool operator==(const refcount_t& other) const {
        return stripe_id == other.stripe_id && count == other.count;
    }
};

using utc_time = std::chrono::time_point<std::chrono::system_clock>;

template <typename T> using coro = boost::asio::traced_awaitable<T>;
inline coro<void> async_noop() { co_return; };

template <typename T> struct is_boost_awaitable : std::false_type {};

template <typename T>
struct is_boost_awaitable<boost::asio::awaitable<T>> : std::true_type {};

template <typename T>
constexpr bool is_boost_awaitable_v = is_boost_awaitable<T>::value;

template <typename Awaitable>
requires is_boost_awaitable_v<std::decay_t<Awaitable>>
inline coro<void> async_wrap(Awaitable&& v) {
    co_await std::move(v);
};

inline thread_local opentelemetry::context::Context THREAD_LOCAL_CONTEXT;

} // end namespace uh::cluster
