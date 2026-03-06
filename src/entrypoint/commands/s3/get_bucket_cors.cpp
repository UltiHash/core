// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "get_bucket_cors.h"

#include "entrypoint/http/string_body.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_bucket_cors::get_bucket_cors(directory& dir)
    : m_dir(dir) {}

bool get_bucket_cors::can_handle(const ep::http::request& req) {
    return req.method() == verb::get && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("cors");
}

coro<response> get_bucket_cors::handle(request& req) {

    auto cors = co_await m_dir.get_bucket_cors(req.bucket());

    if (cors) {
        response r;
        r.set_body(std::make_unique<string_body>(std::move(*cors)));
        co_return r;
    } else {
        co_return response(status::no_content);
    }
}

std::string get_bucket_cors::action_id() const { return "s3:GetBucketCORS"; }

} // namespace uh::cluster
