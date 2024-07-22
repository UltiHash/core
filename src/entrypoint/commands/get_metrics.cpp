#include "get_metrics.h"
#include "common/coroutines/awaitable_promise.h"
#include "config.h"

namespace uh::cluster {

get_metrics::get_metrics(const reference_collection& collection)
    : m_collection(collection) {}

bool get_metrics::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/metrics/cluster";
}

coro<void> get_metrics::handle(http_request& req) const {
    metric<entrypoint_get_metrics_req>::increase(1);
    http_response res;
    auto raw_data_size = co_await m_collection.directory.data_size();
    auto effective_data_size =
        co_await m_collection.gdv.get_used_space(req.m_ctx);

    res.set_body("{\n"
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
                 "}");

    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
