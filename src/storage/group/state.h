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

#include <magic_enum/magic_enum.hpp>

namespace uh::cluster::storage {

enum class storage_state : std::uint8_t { DOWN, NEW, ASSIGNED };

enum class group_state : std::uint8_t {
    UNDETERMINED,
    HEALTHY,
    DEGRADED,
    FAILED,
    REPAIRING
};

} // namespace uh::cluster::storage
