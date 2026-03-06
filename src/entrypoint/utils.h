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

#include <common/crypto/hash.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <entrypoint/http/response.h>
#include <entrypoint/object.h>

#include <optional>
#include <string>
#include <vector>

namespace uh::cluster {

struct collapsed_objects {
    std::optional<std::string> _prefix{};
    std::optional<std::reference_wrapper<const ep::object>> _object{};
};

struct retrieval {
    static std::vector<collapsed_objects>
    collapse(const std::vector<ep::object>& objects,
             std::optional<std::string> delimiter,
             std::optional<std::string> prefix);
};

using encoder_function =
    std::optional<std::string> (*)(std::optional<std::string>);
encoder_function encoder(std::optional<std::string> encoding_type);

/**
 * Set default response headers from object.
 */
void set_default_headers(ep::http::response& res, const ep::object& obj);

/**
 * Deduplicate an HTTP body and compute MD5 checksum
 */
coro<dedupe_response> deduplicate(deduplicator_interface& dd,
                                  ep::http::body& body, md5& hash);

} // namespace uh::cluster
