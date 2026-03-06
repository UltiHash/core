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

#include "abort_multipart.h"
#include "common/telemetry/metrics.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

abort_multipart::abort_multipart(multipart_state& uploads,
                                 storage::global::global_data_view& gdv)
    : m_uploads(uploads),
      m_gdv(gdv) {}

bool abort_multipart::can_handle(const request& req) {
    return req.method() == verb::delete_ &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploadId");
}

coro<response> abort_multipart::handle(request& req) {
    metric<entrypoint_abort_multipart_req>::increase(1);

    auto upload_id = *req.query("uploadId");
    upload_info details;

    {
        auto instance = co_await m_uploads.get();
        auto lock = co_await instance.lock_upload(upload_id);

        details = co_await instance.details(upload_id);
        co_await instance.remove_upload(upload_id);
    }

    for (const auto& part : details.parts) {
        try {
            co_await m_gdv.unlink(part.second.addr);
        } catch (const error_exception& e) {
            LOG_WARN() << req.peer() << ": freeing memory for part "
                       << part.first << " failed: " << e.what();
        }
    }

    co_return response{};
}

std::string abort_multipart::action_id() const {
    return "s3:AbortMultipartUpload";
}

} // namespace uh::cluster
