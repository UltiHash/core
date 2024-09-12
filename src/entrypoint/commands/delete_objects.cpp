#include "delete_objects.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_objects::delete_objects(directory& dir, global_data_view& gdv,
                               limits& uhlimits)
    : m_directory(dir),
      m_gdv(gdv),
      m_limits(uhlimits) {}

bool delete_objects::can_handle(const request& req) {
    return req.method() == verb::post && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && req.object_key().empty() &&
           req.query("delete");
}

namespace {
struct fail {
    std::string key;
    std::string code;
};

response get_response(const std::vector<std::string>& success,
                      const std::vector<fail>& failure) noexcept {
    boost::property_tree::ptree pt;
    boost::property_tree::ptree deleteResult;

    for (const auto& val : success) {
        boost::property_tree::ptree deleted;
        deleted.put("Key", val);
        deleteResult.add_child("Deleted", deleted);
    }
    for (const auto& val : failure) {
        boost::property_tree::ptree error;
        error.put("Key", val.key);
        error.put("Code", val.code);
        deleteResult.add_child("Error", error);
    }

    pt.add_child("DeleteResult", deleteResult);

    response res;
    res << pt;

    return res;
}
} // namespace

coro<response> delete_objects::handle(request& req) {
    metric<entrypoint_delete_objects_req>::increase(1);

    LOG_DEBUG() << req.peer() << ": delete_objects::handle(): content-length: "
                << req.content_length();

    std::vector<char> buffer(req.content_length());
    auto count = co_await req.read_body(buffer);
    buffer.resize(count);

    LOG_DEBUG() << req.peer() << ": delete_objects::handle(): request XML: "
                << std::string(buffer.data(), buffer.data() + buffer.size());

    xml_parser xml_parser;
    bool parsed = xml_parser.parse({&*buffer.begin(), buffer.size()});
    auto object_nodes = xml_parser.get_nodes("Delete.Object");

    if (!parsed || object_nodes.empty() ||
        object_nodes.size() > MAXIMUM_DELETE_KEYS)
        throw command_exception(status::bad_request, "MalformedXML",
                                "xml is invalid");

    auto bucket_id = req.bucket();
    std::vector<std::string> success;
    std::vector<fail> failure;
    for (const auto& objct : object_nodes) {
        auto key = objct.get().get_optional<std::string>("Key");
        if (!key) {
            throw command_exception(status::bad_request, "MalformedXML",
                                    "xml is invalid");
        }

        try {
            LOG_DEBUG() << req.peer() << ": delete_objects::handle(): deleting "
                        << *key;
            try {
                object obj;
                if constexpr (m_enable_refcount) {
                    obj = co_await m_directory.get_object(req.bucket(), *key);
                } else {
                    obj = co_await m_directory.head_object(req.bucket(), *key);
                }

                co_await m_directory.delete_object(req.bucket(), *key);

                if constexpr (m_enable_refcount) {
                    co_await m_gdv.unlink(req.context(), obj.addr.value());
                }

                m_limits.free_storage_size(obj.size);
            } catch (command_exception&) {
            }

            success.emplace_back(*key);

        } catch (const error_exception& e) {
            LOG_ERROR() << req.peer() << ": Failed to delete the bucket "
                        << bucket_id << " to the directory: " << e;
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

    co_return get_response(success, failure);
}

std::string delete_objects::action_id() const { return "s3:DeleteObjects"; }

} // namespace uh::cluster
