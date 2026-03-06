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

#include <common/telemetry/metrics.h>

#include <entrypoint/http/request.h>
#include <entrypoint/http/response.h>

namespace uh::cluster {

class command {
public:
    virtual coro<ep::http::response> handle(ep::http::request&) = 0;
    virtual coro<void> validate(const ep::http::request& req) { co_return; }
    virtual std::string action_id() const = 0;
    virtual ~command() = default;
};

} // end namespace uh::cluster
