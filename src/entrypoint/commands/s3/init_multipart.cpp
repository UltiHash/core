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

#include "init_multipart.h"

#include "entrypoint/http/command_exception.h"

#include <boost/property_tree/ptree.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

response get_response(const request& req,
                      const std::string& upload_id) noexcept {
    response res;

    boost::property_tree::ptree pt;
    pt.put("InitiateMultipartUploadResult.Bucket", req.bucket());
    pt.put("InitiateMultipartUploadResult.Key", req.object_key());
    pt.put("InitiateMultipartUploadResult.UploadId", upload_id);

    res << pt;

    return res;
}

} // namespace

init_multipart::init_multipart(directory& dir, multipart_state& uploads)
    : m_dir(dir),
      m_uploads(uploads) {}

bool init_multipart::can_handle(const request& req) {
    return req.method() == verb::post && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.query("uploads");
}

coro<response> init_multipart::handle(request& req) {
    metric<entrypoint_init_multipart_req>::increase(1);

    co_await m_dir.bucket_exists(req.bucket());

    std::string upload_id;
    {
        auto instance = co_await m_uploads.get();
        upload_id = co_await instance.insert_upload(
            req.bucket(), req.object_key(), req.header("Content-Type"));
    }

    co_return get_response(req, upload_id);
}

std::string init_multipart::action_id() const {
    return "s3:CreateMultipartUpload";
}

} // namespace uh::cluster
