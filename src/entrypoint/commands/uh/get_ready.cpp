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

#include "get_ready.h"
#include "config.h"
#include <entrypoint/http/string_body.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_ready::get_ready(directory& dir, storage::global::global_data_view& gdv)
    : m_dir(dir),
      m_gdv(gdv) {}

bool get_ready::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/ready";
}

coro<response> get_ready::handle(request& req) {
    metric<entrypoint_get_ready_req>::increase(1);
    response res;

    try {
        co_await m_dir.list_buckets();
        co_await m_gdv.get_used_space();
        res.set_body(std::make_unique<string_body>("{\n"
                                                   "  \"ready\": true\n"
                                                   "}"));
    } catch (const std::exception& e) {
        res.set_body(std::make_unique<string_body>("{\n"
                                                   "  \"ready\": false\n"
                                                   "}"));
    }

    co_return res;
}

std::string get_ready::action_id() const { return "uh:GetReady"; }

} // namespace uh::cluster
