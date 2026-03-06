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
#include <common/utils/common.h>

namespace uh::cluster {

struct deduplicator_interface {
    virtual coro<dedupe_response> deduplicate(std::string_view data) = 0;
    static constexpr role service_role = DEDUPLICATOR_SERVICE;
    virtual ~deduplicator_interface() = default;
};

} // namespace uh::cluster
