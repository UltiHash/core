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
