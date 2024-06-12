#include "list_multipart.h"
#include "entrypoint/http/command_exception.h"
#include "common/utils/strings.h"

namespace uh::cluster {

static http_response
get_response(const std::string& bucket_name,
             const std::map<std::string, std::string>& ongoing) noexcept {

    std::string upload_xml_string;

    for (const auto& val : ongoing) {
        upload_xml_string += "<Upload>\n"
                             "<Key>" +
                             xml_escape(val.second) +
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
    return req.method() == method::get && req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && req.query("uploads");
}

coro<void> list_multipart::handle(http_request& req) const {
    metric<entrypoint_list_multipart_req>::increase(1);
    const std::string& bucket_name = req.bucket();

    auto ongoing =
        co_await m_collection.uploads.list_multipart_uploads(bucket_name);
    if (ongoing.empty()) {
        throw command_exception(http::status::not_found, "NoMultiPartUploads",
                                "no multipart uploads");
    }

    auto res = get_response(bucket_name, ongoing);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
