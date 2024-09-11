#include "get_metrics.h"
#include "config.h"

namespace uh::cluster {

get_metrics::get_metrics(directory& dir, global_data_view& gdv)
    : m_directory(dir),
      m_gdv(gdv) {}

bool get_metrics::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/metrics/cluster";
}

coro<http_response> get_metrics::handle(http_request& req) {
    metric<entrypoint_get_metrics_req>::increase(1);
    auto raw_data_size = co_await m_directory.data_size();
    auto effective_data_size = co_await m_gdv.get_used_space(req.context());

    http_response res;
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
