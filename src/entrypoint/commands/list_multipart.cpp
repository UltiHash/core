#include "list_multipart.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

static http_response
get_response(const std::string& bucket_name,
             const std::map<std::string, std::string>& ongoing) noexcept {

    std::string upload_xml_string;

    for (const auto& val : ongoing) {
        upload_xml_string += "<Upload>\n"
                             "<Key>" +
                             val.second +
                             "</Key>\n"
                             "<UploadId>" +
                             val.first +
                             "</UploadId>\n"
                             "</Upload>\n";
    }

    http_response res;
    res.set_body(std::string("<ListMultipartUploadsResult>\n"
                             "   <Bucket>" +
                             bucket_name + "</Bucket>\n" + upload_xml_string +
                             "</ListMultipartUploadsResult>"));

    return res;
}

list_multipart::list_multipart(const reference_collection& collection)
    : m_collection(collection) {}

bool list_multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.query_string_exists("uploads");
}

coro<http_response> list_multipart::handle(const http_request& req) const {
    metric<entrypoint_list_multipart_req>::increase(1);
    const std::string& bucket_name = req.get_uri().get_bucket_id();

    std::map<std::string, std::string> ongoing{};
    auto func = [](const reference_collection& collection,
                   const std::string& bucket_name,
                   std::map<std::string, std::string>& ongoing) {
        ongoing = collection.server_state.m_uploads.list_multipart_uploads(
            bucket_name);

        if (ongoing.empty()) {
            throw command_exception(http::status::not_found,
                                    command_error::no_mp_uploads);
        }
    };

    co_await worker_utils::post_in_workers(
        m_collection.workers, m_collection.ioc,
        std::bind_front(func, std::ref(m_collection), std::cref(bucket_name),
                        std::ref(ongoing)));

    co_return get_response(bucket_name, ongoing);
}

} // namespace uh::cluster
