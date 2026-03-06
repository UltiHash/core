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

#include "delete_bucket.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_bucket::delete_bucket(directory& dir)
    : m_dir(dir) {}

bool delete_bucket::can_handle(const request& req) {
    return req.method() == verb::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && !req.query("policy") &&
           !req.query("cors");
}

coro<response> delete_bucket::handle(request& req) {
    metric<entrypoint_delete_bucket_req>::increase(1);

    co_await m_dir.delete_bucket(req.bucket());

    co_return response{};
}

std::string delete_bucket::action_id() const { return "s3:DeleteBucket"; }

} // namespace uh::cluster
