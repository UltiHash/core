#include "get_metrics.h"
#include "config.h"
#include <entrypoint/http/string_body.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_metrics::get_metrics(directory& dir, sn::interface& gdv)
    : m_dir(dir),
      m_gdv(gdv) {}

bool get_metrics::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/metrics/cluster";
}

coro<response> get_metrics::handle(request& req) {
    metric<entrypoint_get_metrics_req>::increase(1);
    auto raw_data_size = co_await m_dir.data_size();
    auto effective_data_size = co_await m_gdv.get_used_space(req.context());

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
