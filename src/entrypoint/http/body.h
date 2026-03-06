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

#include <common/types/common_types.h>
#include <span>

namespace uh::cluster::ep::http {

class body {
public:
    virtual ~body() = default;

    /**
     * Return remaining length of body if available.
     */
    virtual std::optional<std::size_t> length() const = 0;

    /**
     * Read next chunk of data. May return less then `count` bytes.
     * Return zero-length span on end of body.
     */
    virtual coro<std::span<const char>> read(std::size_t count) = 0;

    /**
     * Clear underlying stream buffers
     */
    virtual coro<void> consume() = 0;

    virtual std::size_t buffer_size() const = 0;
};

template <typename container = std::string>
coro<container> copy_to_buffer(body& s) {
    std::size_t bs = s.buffer_size();

    container rv;
    std::size_t len = 0;

    std::span<const char> data = co_await s.read(bs);

    while (!data.empty()) {
        rv.resize(len + data.size());
        memcpy(&rv[len], data.data(), data.size());
        len += data.size();

        co_await s.consume();
        data = co_await s.read(bs);
    }

    co_await s.consume();
    co_return std::move(rv);
}

} // namespace uh::cluster::ep::http
