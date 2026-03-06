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

#include <common/etcd/namespace.h>

namespace uh::cluster::storage {

using prefix_t = ns::storage_groups_t::impl_t;

inline prefix_t get_prefix(size_t group_id) {
    return ns::root.storage_groups[group_id];
}

using offset_prefix_t = ns::subscriptable_key_t;

inline offset_prefix_t get_storage_offset_prefix(size_t group_id) {
    return ns::root.storage_groups.temporaries[group_id].storage_offsets;
}

} // namespace uh::cluster::storage
