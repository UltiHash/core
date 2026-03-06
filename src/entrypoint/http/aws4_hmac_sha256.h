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

#include <entrypoint/http/request.h>
#include <entrypoint/user/db.h>
#include <set>

namespace uh::cluster::ep::http {

struct aws4_signature_info {
    std::string date;
    std::string region;
    std::string service;
    std::set<std::string> signed_headers;
    std::string amz_date;
    std::string content_sha;
    std::set<std::string>& query_ignore;
};

class aws4_hmac_sha256 {
public:
    static coro<std::unique_ptr<request>>
    create(stream& s, user::db& users, raw_request req,
           const std::string& auth);

    static coro<std::unique_ptr<request>>
    create_from_url(stream& s, user::db& users, raw_request req);
};

} // namespace uh::cluster::ep::http
