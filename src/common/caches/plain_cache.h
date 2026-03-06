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

#include "common/types/scoped_buffer.h"
#include <functional>
#include <zpp_bits.h>

namespace uh::cluster {

class plain_cache {
    std::vector<char> m_memory;
    const size_t m_capacity;
    size_t m_elements{0};
    std::function<void(std::span<char>)> m_flush_callback;

public:
    explicit plain_cache(size_t capacity,
                         std::function<void(std::span<char>)> flush_callback)
        : m_capacity(capacity),
          m_flush_callback{std::move(flush_callback)} {}

    inline plain_cache& operator<<(const auto& obj) {
        zpp::bits::out{m_memory, zpp::bits::size4b{}, zpp::bits::append{}}(obj)
            .or_throw();
        m_elements++;
        if (m_elements == m_capacity) {
            flush();
        }
        return *this;
    }

    inline void flush() {
        m_flush_callback(m_memory);
        m_memory.clear();
        m_elements = 0;
    }

    ~plain_cache() { flush(); }
};

} // namespace uh::cluster
