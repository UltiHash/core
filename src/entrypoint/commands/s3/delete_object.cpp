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

#include "delete_object.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_object::delete_object(directory& dir,
                             storage::global::global_data_view& gdv,
                             limits& uhlimits)
    : m_dir(dir) {}

bool delete_object::can_handle(const request& req) {
    return req.method() == verb::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("uploadId");
}

coro<response> delete_object::handle(request& req) {
    metric<entrypoint_delete_object_req>::increase(1);

    co_await m_dir.bucket_exists(req.bucket());
    auto result = co_await m_dir.delete_object(req.bucket(), req.object_key(), req.query("versionId"));

    response res;

    res.set("X-Amz-Delete-Marker", result.is_delete_marker ? "true" : "false");
    res.set("X-Amz-Version-Id", result.version);

    co_return res;
}

std::string delete_object::action_id() const { return "s3:DeleteObject"; }

} // namespace uh::cluster
