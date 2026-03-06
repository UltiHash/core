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

#include <entrypoint/http/raw_request.h>
#include <set>
#include <string>

namespace uh::cluster::ep::cors {

struct info {
    std::string origin;
    std::set<http::verb> methods;
    std::set<std::string> headers;
    std::optional<std::string> expose_headers;
    std::optional<unsigned> max_age_seconds;
};

} // namespace uh::cluster::ep::cors
