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

#include <common/types/address.h>
#include <string>

namespace uh::cluster::proxy::cache::disk {

struct object_metadata {
    std::string path;
    std::string version;

    bool operator==(const object_metadata& other) const {
        return path == other.path && version == other.version;
    }
};

} // namespace uh::cluster::proxy::cache::disk

template <> struct std::hash<uh::cluster::proxy::cache::disk::object_metadata> {
    size_t operator()(
        const uh::cluster::proxy::cache::disk::object_metadata& key) const {
        std::size_t seed = 0;

        auto hash_combine = [](std::size_t& seed, const auto& v) {
            seed ^= std::hash<std::remove_cvref_t<decltype(v)>>{}(v) +
                    0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

        hash_combine(seed, key.path);
        hash_combine(seed, key.version);

        return seed;
    }
};

namespace uh::cluster::proxy::cache::disk {

struct object_handle {
    object_handle() = default;
    object_handle(address&& a, std::size_t header_size = 0)
        : m_addr(std::move(a)),
          m_header_size(header_size) {}

    object_handle(object_handle&&) = default;
    object_handle& operator=(object_handle&&) = default;

    std::size_t header_size() const { return m_header_size; }

    std::size_t data_size() const { return m_addr.data_size(); }

    const address& get_address() const { return m_addr; }

private:
    address m_addr;
    std::size_t m_header_size;
};

} // namespace uh::cluster::proxy::cache::disk
