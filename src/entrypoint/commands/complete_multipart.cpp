#include "complete_multipart.h"
#include "common/coroutines/coro_utils.h"
#include "common/utils/md5.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_response.h"

namespace uh::cluster {

complete_multipart::complete_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool complete_multipart::can_handle(const http_request& req) {
    return req.method() == method::post && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploadId");
}

void complete_multipart::validate(const http_request& req,
                                  const std::span<char> body) const {
    auto up_info = m_collection.server_state.m_uploads.get_upload_info(
        *req.query("uploadId"));

    xml_parser xml_parser;
    bool parsed = xml_parser.parse({&*body.begin(), body.size()});
    auto part_nodes = xml_parser.get_nodes("CompleteMultipartUpload.Part");

    if (!parsed || part_nodes.empty())
        throw command_exception(http::status::bad_request, "MalformedXML",
                                "xml is invalid");

    for (uint16_t part_counter = 1; const auto& part : part_nodes) {
        auto part_num = part.get().get_optional<std::size_t>("PartNumber");
        auto etag = part.get().get_optional<std::string>("ETag");

        if (!part_num || !etag || part_counter > MAXIMUM_PART_NUMBER)
            throw command_exception(http::status::bad_request, "MalformedXML",
                                    "xml is invalid");

        if (*part_num != part_counter) {
            throw command_exception(http::status::bad_request,
                                    "InvalidPartOrder",
                                    "part order is not ascending");
        }

        if (up_info->part_sizes.at(*part_num) < MAXIMUM_CHUNK_SIZE and
            part_num != up_info->part_sizes.size()) {
            throw command_exception(http::status::bad_request, "EntityTooSmall",
                                    "entity is too small");
        }

        if (up_info->etags.at(*part_num) != etag) {
            throw command_exception(http::status::bad_request, "InvalidPart",
                                    "part not found");
        }

        part_counter++;
    }
}

coro<void> complete_multipart::handle(http_request& req) const {
    metric<entrypoint_complete_multipart_req>::increase(1);

    unique_buffer<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer.span());
    buffer.resize(size);

    validate(req, buffer.span());

    auto upload_id = *req.query("uploadId");
    const auto& bucket_name = req.bucket();
    const auto& object_name = req.object_key();

    const auto& up_info =
        m_collection.server_state.m_uploads.get_upload_info(upload_id);

    auto func = [&](std::shared_ptr<directory_interface> dir,
                    size_t id) -> coro<void> {
        co_await dir->put_object(bucket_name, object_name,
                                 up_info->generate_total_address());
    };

    co_await broadcast<directory_interface>(
        m_collection.ioc, func, m_collection.directory_services.get_services());

    metric<entrypoint_ingested_data_counter, byte>::increase(
        up_info->data_size);

    auto etag = calculate_md5(buffer.span());
    http_response res;
    res.set_etag(etag);
    res.set_original_size(up_info->data_size);
    res.set_effective_size(up_info->effective_size);

    res.set_body("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<CompleteMultipartUploadResult>\n"
                 "<Bucket>" +
                 up_info->bucket +
                 "</Bucket>\n"
                 "<Key>" +
                 up_info->key +
                 "</Key>\n"
                 "<ETag>" +
                 etag +
                 "</ETag>\n"
                 "</CompleteMultipartUploadResult>\n");

    m_collection.server_state.m_uploads.remove_upload(upload_id);

    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
