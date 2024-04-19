#include "complete_multipart.h"
#include "common/utils/md5.h"
#include "common/utils/worker_pool.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

complete_multipart::complete_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool complete_multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::post && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() && uri.query_string_exists("uploadId");
}

void complete_multipart::validate(const http_request& req,
                                  const std::vector<char>& body) const {
    const auto& upload_id = req.get_uri().get_query_parameters().at("uploadId");
    if (upload_id.empty()) {
        throw command_exception(http::status::bad_request,
                                command_error::type::bad_upload_id);
    }

    const auto up_info =
        m_collection.server_state.m_uploads.get_upload_info(upload_id);
    if (up_info == nullptr) {
        throw command_exception(http::status::not_found,
                                command_error::type::no_such_upload);
    }

    xml_parser xml_parser;
    bool parsed = xml_parser.parse({&*body.begin(), body.size()});
    auto part_nodes = xml_parser.get_nodes("CompleteMultipartUpload.Part");

    if (!parsed || part_nodes.empty())
        throw command_exception(http::status::bad_request,
                                command_error::type::malformed_xml);

    for (uint16_t part_counter = 1; const auto& part : part_nodes) {
        auto part_num = part.get().get_optional<std::size_t>("PartNumber");
        auto etag = part.get().get_optional<std::string>("ETag");

        if (!part_num || !etag || part_counter > MAXIMUM_PART_NUMBER)
            throw command_exception(http::status::bad_request,
                                    command_error::type::malformed_xml);

        if (*part_num != part_counter) {
            throw command_exception(http::status::bad_request,
                                    command_error::type::invalid_part_oder);
        }

        if (up_info->part_sizes.at(*part_num) < MAXIMUM_CHUNK_SIZE and
            part_num != up_info->part_sizes.size()) {
            throw command_exception(http::status::bad_request,
                                    command_error::type::entity_too_small);
        }

        if (up_info->etags.at(*part_num) != etag) {
            throw command_exception(http::status::bad_request,
                                    command_error::type::invalid_part);
        }

        part_counter++;
    }
}

coro<void> complete_multipart::handle(http_request& req) const {
    metric<entrypoint_complete_multipart_req>::increase(1);

    std::vector<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer);
    buffer.resize(size);

    validate(req, buffer);

    const auto& req_uri = req.get_uri();
    const auto& upload_id = req_uri.get_query_parameters().at("uploadId");
    const auto& bucket_name = req.get_uri().get_bucket_id();
    const auto& object_name = req_uri.get_object_key();

    const auto& up_info =
        m_collection.server_state.m_uploads.get_upload_info(upload_id);

    const directory_message dir_req{
        .bucket_id = bucket_name,
        .object_key = std::make_unique<std::string>(object_name),
        .addr = std::make_unique<address>(up_info->generate_total_address()),
    };

    auto func = [&](acquired_messenger<coro_client> m,
                    std::size_t id) -> coro<void> {
        co_await m->send_directory_message(DIRECTORY_OBJECT_PUT_REQ, dir_req);
        co_await m->recv_header();
    };

    co_await m_collection.directory_services.broadcast(func);

    metric<entrypoint_ingested_data_counter, byte>::increase(
        up_info->data_size);

    auto etag = calculate_md5(buffer);
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
