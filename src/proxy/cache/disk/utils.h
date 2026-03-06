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

/*
 * Disk utilities: APIs for common disk operations
 */
#pragma once

#include <common/service_interfaces/deduplicator_interface.h>
#include <common/types/common_types.h>
#include <storage/interfaces/data_view.h>

#include <memory>

namespace uh::cluster::proxy::cache::disk::utils {

inline coro<void> erase(storage::data_view& storage, const address& addr) {
    co_await storage.unlink(addr);
}

inline coro<address> store(storage::data_view& storage,
                           std::span<const char> sv) {
    auto addr =
        co_await storage.write(std::string_view{sv.data(), sv.size()}, {0});
    co_return std::move(addr);
}

inline coro<void> read(storage::data_view& storage, const address& addr,
                       std::span<char> sv) {
    co_await storage.read_address(addr, sv);
}

} // namespace uh::cluster::proxy::cache::disk::utils
