#include "delete_objects.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

delete_objects::delete_objects(const reference_collection& collection)
    : m_collection(collection) {}

bool delete_objects::can_handle(const http_request& req) {
    return req.method() == method::post && !req.bucket().empty() &&
           req.object_key().empty() && req.query("delete");
}

namespace {
struct fail {
    std::string key;
    std::string code;
};

http_response get_response(const std::vector<std::string>& success,
                           const std::vector<fail>& failure) noexcept {
    http_response res;

    std::string xml_string;
    for (const auto& val : success) {
        xml_string += "<Deleted>\n"
                      "<Key>" +
                      val +
                      "</Key>\n"
                      "</Deleted>\n";
    }
    for (const auto& val : failure) {
        xml_string += "<Error>\n"
                      "<Key>" +
                      val.key +
                      "</Key>\n"
                      "<Code>" +
                      val.code +
                      "</Code>\n"
                      "</Error>\n";
    }

    res.set_body(std::string("<DeleteResult>\n" + std::move(xml_string) +
                             "</DeleteResult>"));

    return res;
}
} // namespace

coro<void> delete_objects::handle(http_request& req) const {
    metric<entrypoint_delete_objects_req>::increase(1);

    LOG_DEBUG() << "delete_objects::handle(): content-length: "
                << req.content_length();

    std::vector<char> buffer(req.content_length());
    auto count = co_await req.read_body(buffer);
    buffer.resize(count);

    LOG_DEBUG() << "delete_objects::handle(): request XML: "
                << std::string(buffer.data(), buffer.data() + buffer.size());

    xml_parser xml_parser;
    bool parsed = xml_parser.parse({&*buffer.begin(), buffer.size()});
    auto object_nodes = xml_parser.get_nodes("Delete.Object");

    if (!parsed || object_nodes.empty() ||
        object_nodes.size() > MAXIMUM_DELETE_KEYS)
        throw command_exception(http::status::bad_request, "MalformedXML",
                                "xml is invalid");

    auto bucket_id = req.bucket();
    std::vector<std::string> success;
    std::vector<fail> failure;
    for (const auto& object : object_nodes) {
        auto key = object.get().get_optional<std::string>("Key");
        if (!key) {
            throw command_exception(http::status::bad_request, "MalformedXML",
                                    "xml is invalid");
        }

        try {
            LOG_DEBUG() << "delete_objects::handle(): deleting " << *key;

            co_await m_collection.directory.delete_object(req.bucket(), *key);
            success.emplace_back(*key);

        } catch (const error_exception& e) {
            LOG_ERROR() << "Failed to delete the bucket " << bucket_id
                        << " to the directory: " << e;
            switch (*e.error()) {
            case error::object_not_found:
                failure.emplace_back(*key, "NoSuchKey");
                break;
            case error::bucket_not_found:
                failure.emplace_back(*key, "NoSuchBucket");
                break;
            default:
                failure.emplace_back(*key);
            }
        }
    }

    auto res = get_response(success, failure);
    LOG_DEBUG() "delete_objects: " << res;
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
