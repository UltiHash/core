#include "get_metrics.h"
#include "config.h"

namespace uh::cluster {

get_metrics::get_metrics(const reference_collection& collection)
    : m_collection(collection) {}

bool get_metrics::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/metrics/cluster";
}

coro<http_response> get_metrics::handle(http_request& req) const {
    metric<entrypoint_get_metrics_req>::increase(1);
    auto raw_data_size = co_await m_collection.directory.data_size();
    auto effective_data_size =
        co_await m_collection.gdv.get_used_space(req.context());

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

} // namespace uh::cluster
