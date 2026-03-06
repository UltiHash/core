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

#include "create_bucket.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

create_bucket::create_bucket(directory& dir)
    : m_dir(dir) {}

bool create_bucket::can_handle(const request& req) {
    return req.method() == verb::put && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && req.object_key().empty() &&
           !req.query("policy") && !req.query("cors") &&
           !req.query("versioning");
}

coro<response> create_bucket::handle(request& req) {
    metric<entrypoint_create_bucket_req>::increase(1);
    auto bucket_id = req.bucket();
    co_await m_dir.put_bucket(bucket_id);
    co_return response{};
}

std::string create_bucket::action_id() const { return "s3:CreateBucket"; }

} // namespace uh::cluster
