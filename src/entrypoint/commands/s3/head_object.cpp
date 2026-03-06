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

#include "head_object.h"

#include <entrypoint/utils.h>
#include <entrypoint/http/command_exception.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

head_object::head_object(directory& dir)
    : m_dir(dir) {}

bool head_object::can_handle(const request& req) {
    return req.method() == verb::head && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           !req.query("attributes");
}

coro<response> head_object::handle(request& req) {
    metric<entrypoint_head_object_req>::increase(1);

    try {
        auto obj = co_await m_dir.head_object(req.bucket(), req.object_key(), req.query("versionId"));

        response res;
        set_default_headers(res, obj);
        res.set("Content-Length", std::to_string(obj.size));

        co_return res;
    } catch (const std::exception& e) {
        throw command_exception(status::not_found, "NoSuchKey",
                                "Object not found.");
    }
}

std::string head_object::action_id() const { return "s3:HeadObject"; }

} // namespace uh::cluster
