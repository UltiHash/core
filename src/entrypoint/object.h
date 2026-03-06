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

#include <string>

namespace uh::cluster::ep {

enum class object_state {
    normal, deleted, collected
};

struct object {
    uint64_t id;
    std::string name;
    std::size_t size{};
    utc_time last_modified;

    std::optional<address> addr;
    std::optional<std::string> etag;
    std::optional<std::string> mime;
    std::optional<std::string> version;

    object_state state = object_state::normal;
};

object_state to_object_state(const std::string& s);
std::string to_string(object_state os);

} // namespace uh::cluster::ep
