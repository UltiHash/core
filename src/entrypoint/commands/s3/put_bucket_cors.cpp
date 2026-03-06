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

#include "put_bucket_cors.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

put_bucket_cors::put_bucket_cors(directory& dir)
    : m_dir(dir) {}

bool put_bucket_cors::can_handle(const ep::http::request& req) {
    return req.method() == verb::put && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("cors");
}

coro<response> put_bucket_cors::handle(request& req) {

    co_await m_dir.set_bucket_cors(req.bucket(), co_await copy_to_buffer(req.body()));
    co_return response(status::no_content);
}

std::string put_bucket_cors::action_id() const { return "s3:PutBucketCORS"; }

} // namespace uh::cluster
