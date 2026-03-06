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

#include "get_metrics.h"
#include "config.h"
#include <entrypoint/http/string_body.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_metrics::get_metrics(directory& dir, storage::global::global_data_view& gdv)
    : m_dir(dir),
      m_gdv(gdv) {}

bool get_metrics::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/metrics/cluster";
}

coro<response> get_metrics::handle(request& req) {
    metric<entrypoint_get_metrics_req>::increase(1);
    auto raw_data_size = co_await m_dir.data_size();
    auto effective_data_size = co_await m_gdv.get_used_space();

    response res;
    res.set_body(
        std::make_unique<string_body>("{\n"
                                      "  \"version\": \"" +
                                      std::string(PROJECT_VERSION) +
                                      "\",\n"
                                      "  \"vcsid\": \"" +
                                      std::string(PROJECT_VCSID) +
                                      "\",\n"
                                      "  \"raw_data_size\": " +
                                      std::to_string(raw_data_size) +
                                      ",\n"
                                      "  \"effective_data_size\": " +
                                      std::to_string(effective_data_size) +
                                      "\n"
                                      "}"));

    co_return res;
}

std::string get_metrics::action_id() const { return "uh:GetMetrics"; }

} // namespace uh::cluster
