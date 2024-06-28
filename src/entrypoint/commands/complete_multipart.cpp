#include "complete_multipart.h"
#include "common/types/common_types.h"
#include "common/utils/md5.h"
#include "common/utils/strings.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_response.h"

namespace uh::cluster {

complete_multipart::complete_multipart(reference_collection& collection)
    : m_collection(collection) {}

bool complete_multipart::can_handle(const http_request& req) {
    return req.method() == method::post &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && req.query("uploadId");
}

void complete_multipart::validate(const upload_info& info,
                                  std::span<char> body) const {
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

        auto it = info.parts.find(*part_num);
        if (it == info.parts.end()) {
            throw command_exception(http::status::bad_request, "InvalidPart",
                                    "part not found");
        }

        const upload_info::part& pt = it->second;

        if (pt.size < MAXIMUM_CHUNK_SIZE && *part_num != info.parts.size()) {
            throw command_exception(http::status::bad_request, "EntityTooSmall",
                                    "entity is too small");
        }

        if (pt.etag != etag) {
            throw command_exception(http::status::bad_request, "InvalidPart",
                                    "part etag does not match");
        }

        part_counter++;
    }
}

coro<void> complete_multipart::handle(http_request& req) const {
    metric<entrypoint_complete_multipart_req>::increase(1);

    unique_buffer<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer.span());
    buffer.resize(size);

    auto upload_id = *req.query("uploadId");
    const auto info = co_await m_collection.uploads.details(upload_id);

    validate(info, buffer.span());

    m_collection.limits.check_storage_size(info.data_size);

    auto etag = calculate_md5(buffer.span());

    auto addr = info.generate_total_address();
    object obj{.name = req.object_key(),
               .size = addr.data_size(),
               .addr = std::move(addr),
               .etag = etag};
    co_await m_collection.directory.put_object(req.bucket(), obj);

    metric<entrypoint_ingested_data_counter, byte>::increase(info.data_size);

    http_response res;
    res.set_etag(etag);
    res.set_original_size(info.data_size);
    res.set_effective_size(info.effective_size);

    boost::property_tree::ptree pt;
    pt.put("CompleteMultipartUploadResult.Bucket", req.bucket());
    pt.put("CompleteMultipartUploadResult.Key", info.key);
    pt.put("CompleteMultipartUploadResult.ETag", etag);
    res << pt;

    co_await m_collection.uploads.remove_upload(upload_id);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
