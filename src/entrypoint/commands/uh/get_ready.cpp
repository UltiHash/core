#include "get_ready.h"
#include "config.h"
#include <entrypoint/http/string_body.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

get_ready::get_ready(directory& dir, global_data_view& gdv)
    : m_dir(dir),
      m_gdv(gdv) {}

bool get_ready::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() == RESERVED_BUCKET_NAME &&
           req.object_key() == "v1/ready";
}

coro<response> get_ready::handle(request& req) {
    metric<entrypoint_get_ready_req>::increase(1);
    response res;

    try {
        co_await m_dir.list_buckets();
        co_await m_gdv.get_used_space(req.context());
        res.set_body(std::make_unique<string_body>("{\n"
                                                   "  \"ready\": true\n"
                                                   "}"));
    } catch (const std::exception& e) {
        res.set_body(std::make_unique<string_body>("{\n"
                                                   "  \"ready\": false\n"
                                                   "}"));
    }

    co_return res;
}

std::string get_ready::action_id() const { return "uh:GetReady"; }

} // namespace uh::cluster
