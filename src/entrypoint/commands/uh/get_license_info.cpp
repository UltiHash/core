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

#include "get_license_info.h"

#include <common/telemetry/metrics.h>
#include <entrypoint/http/string_body.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_license_info::get_license_info(license_watcher& w)
    : m_watcher(w) {}

bool get_license_info::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/license";
}

coro<response> get_license_info::handle(request& req) {
    metric<entrypoint_get_license_info_req>::increase(1);

    response res;
    res.set_body(
        std::make_unique<string_body>(m_watcher.get_license()->to_string()));

    co_return res;
}

std::string get_license_info::action_id() const { return "uh:GetLicenseInfo"; }

} // namespace uh::cluster
