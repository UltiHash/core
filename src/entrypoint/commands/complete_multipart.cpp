#include "complete_multipart.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/utils/xml_parser.h"

namespace uh::cluster {

complete_multipart::complete_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool complete_multipart::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::post && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() && uri.query_string_exists("uploadId");
}

void complete_multipart::validate(const http_request& req) const {
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
    bool parsed = xml_parser.parse(req.get_body());
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
            part_num != up_info->part_sizes.size() - 1) {
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

coro<http_response> complete_multipart::handle(http_request& req) const {
    metric<entrypoint_complete_multipart_req>::increase(1);

    co_await req.read_body();
    validate(req);

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

    auto func_dir = [](const directory_message& dir_req,
                       client::acquired_messenger m, long id) -> coro<void> {
        co_await m.get().send_directory_message(DIRECTORY_OBJECT_PUT_REQ,
                                                dir_req);
        co_await m.get().recv_header();
    };

    co_await worker_utils::broadcast_from_io_thread_in_io_threads(
        m_collection.directory_services.get_clients(), m_collection.ioc,
        m_collection.workers, std::bind_front(func_dir, std::cref(dir_req)));

    const auto size_mb = static_cast<double>(up_info->data_size) / MEBI_BYTE;
    auto effective_size =
        static_cast<double>(up_info->effective_size) / MEBI_BYTE;
    auto space_saving = 1.0 - effective_size / size_mb;
    const auto stop = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
    const auto dur_ms = stop - up_info->upload_init_time;

    const double dur_s = static_cast<double>(dur_ms) / 1000.0;
    const auto bandwidth = size_mb / dur_s;

    metric<entrypoint_ingested_data_counter, byte>::increase(
        up_info->data_size);

    LOG_DEBUG() << "upload size: " << req.get_body_size();
    LOG_DEBUG() << "original size " << size_mb << " MB";
    LOG_DEBUG() << "effective size " << effective_size << " MB";
    LOG_DEBUG() << "space saving " << space_saving;
    LOG_DEBUG() << "integration duration " << dur_s << " s";
    LOG_DEBUG() << "integration bandwidth " << bandwidth << " MB/s";

    http_response res;
    res.set_etag(md5::calculateMD5(req.get_body()));
    res.set_original_size(up_info->data_size);
    res.set_effective_size(up_info->effective_size);
    res.set_space_savings(space_saving);
    res.set_bandwidth(bandwidth);

    m_collection.server_state.m_uploads.remove_upload(upload_id, bucket_name,
                                                      object_name);

    co_return std::move(res);
}

} // namespace uh::cluster
