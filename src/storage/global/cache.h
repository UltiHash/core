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

#include <common/caches/lru_cache.h>
#include <storage/interfaces/data_view.h>

namespace uh::cluster::storage::global {

class cache {
public:
    cache(boost::asio::io_context& ioc, data_view& storage,
          std::size_t capacity);

    shared_buffer<> read_fragment(const uint128_t& pointer, size_t size);

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size);

private:
    boost::asio::io_context& m_ioc;
    storage::data_view& m_storage;
    lru_cache<uint128_t, shared_buffer<char>> m_lru;
};

} // namespace uh::cluster::storage::global
