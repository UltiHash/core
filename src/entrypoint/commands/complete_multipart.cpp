#include "complete_multipart.h"
#include "common/crypto/hash.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"
#include <entrypoint/constant.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

constexpr std::size_t MAXIMUM_CHUNK_SIZE = 5ul * 1024ul * 1024ul;
constexpr std::size_t MAXIMUM_PART_NUMBER = 10000;

void validate_internal(const upload_info& info, std::span<char> body) {
    xml_parser xml_parser;
    bool parsed = xml_parser.parse({&*body.begin(), body.size()});
    auto part_nodes = xml_parser.get_nodes("CompleteMultipartUpload.Part");

    if (!parsed || part_nodes.empty())
        throw command_exception(status::bad_request, "MalformedXML",
                                "xml is invalid");

    for (uint16_t part_counter = 1; const auto& part : part_nodes) {
        auto part_num = part.get().get_optional<std::size_t>("PartNumber");
        auto etag = part.get().get_optional<std::string>("ETag");

        if (!part_num || !etag || part_counter > MAXIMUM_PART_NUMBER)
            throw command_exception(status::bad_request, "MalformedXML",
                                    "xml is invalid");

        auto it = info.parts.find(*part_num);
        if (it == info.parts.end()) {
            throw command_exception(status::bad_request, "InvalidPart",
                                    "part not found");
        }

        const upload_info::part& pt = it->second;

        if (pt.size < MAXIMUM_CHUNK_SIZE && *part_num != info.parts.size()) {
            throw command_exception(status::bad_request, "EntityTooSmall",
                                    "entity is too small");
        }

        if (pt.etag != etag) {
            throw command_exception(status::bad_request, "InvalidPart",
                                    "part etag does not match");
        }

        if (*part_num != part_counter) {
            throw command_exception(status::bad_request, "InvalidPartOrder",
                                    "part order is not ascending");
        }

        part_counter++;
    }
}

std::string multipart_etag(const upload_info& info) {
    std::string buffer;

    for (const auto& part : info.parts) {
        buffer += unhex(part.second.etag);
    }

    return to_hex(md5::from_string(buffer)) + "-" +
           std::to_string(info.parts.size());
}

} // namespace

complete_multipart::complete_multipart(directory& dir, global_data_view& gdv,
                                       multipart_state& uploads,
                                       limits& uhlimits)
    : m_dir(dir),
      m_gdv(gdv),
      m_uploads(uploads),
      m_limits(uhlimits) {}

bool complete_multipart::can_handle(const request& req) {
    return req.method() == verb::post && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           req.query("uploadId");
}

coro<response> complete_multipart::handle(request& req) {
    metric<entrypoint_complete_multipart_req>::increase(1);

    unique_buffer<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer.span());
    buffer.resize(size);

    upload_info info;
    std::string id = *req.query("uploadId");
    std::string etag;

    {
        auto instance = co_await m_uploads.get();
        auto lock = co_await instance.lock_upload(id);
        info = co_await instance.details(id);

        validate_internal(info, buffer.span());

        if (!info.completed) {
            m_limits.check_storage_size(info.data_size);
        }

        etag = multipart_etag(info);

        auto addr = info.generate_total_address();
        object obj{.name = req.object_key(),
                   .size = addr.data_size(),
                   .addr = std::move(addr),
                   .etag = etag,
                   .mime = info.mime.value_or(ep::DEFAULT_OBJECT_CONTENT_TYPE)};

        if (!info.completed) {
            co_await m_dir.put_object(req.bucket(), obj);
            co_await instance.remove_upload(id);
        }
    }

    metric<entrypoint_ingested_data_counter, byte>::increase(info.data_size);

    response res;
    res.set("ETag", etag);
    res.set_original_size(info.data_size);
    res.set_effective_size(info.effective_size);

    boost::property_tree::ptree pt;
    pt.put("CompleteMultipartUploadResult.Bucket", req.bucket());
    pt.put("CompleteMultipartUploadResult.Key", info.key);
    pt.put("CompleteMultipartUploadResult.ETag", etag);
    res << pt;

    co_return res;
}

std::string complete_multipart::action_id() const {
    return "s3:CompleteMultipartUpload";
}

} // namespace uh::cluster
