#include "multipart.h"
#include "common/utils/md5.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

multipart::multipart(reference_collection& collection)
    : m_collection(collection) {}

bool multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::put && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() &&
           uri.query_string_exists("partNumber") &&
           uri.query_string_exists("uploadId");
}

static void validate(const http_request& req) {
    const auto& req_uri = req.get_uri();

    auto upload_id = req_uri.get_query_parameters().at("uploadId");
    if (upload_id.empty()) {
        throw command_exception(boost::beast::http::status::bad_request,
                                command_error::type::bad_upload_id);
    }

    auto part_num = std::stoi(req_uri.get_query_parameters().at("partNumber"));
    if (part_num < 1 || part_num > 10000) {
        throw command_exception(boost::beast::http::status::bad_request,
                                command_error::type::bad_part_number);
    }
}

coro<void> multipart::handle(http_request& req) const {
    metric<entrypoint_multipart_req>::increase(1);

    validate(req);
    std::vector<char> buffer(req.content_length());

    auto size = co_await req.read_body(buffer);
    buffer.resize(size);

    dedupe_response resp = {};
    if (buffer.size() > 0) {
        resp = co_await integration::integrate_data(buffer, m_collection);
    }

    m_collection.server_state.m_uploads.append_upload_part_info(
        req.get_uri().get_query_parameters().at("uploadId"),
        std::stoi(req.get_uri().get_query_parameters().at("partNumber")), resp,
        buffer);

    http_response res;
    res.set_etag(calculate_md5(buffer));

    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
