#include "delete_objects.h"
#include "common/utils/xml_parser.h"
#include "entrypoint/http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

delete_objects::delete_objects(directory& dir,
                               storage::global::global_data_view& gdv,
                               limits& uhlimits)
    : m_dir(dir) {}

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

    co_await m_dir.bucket_exists(req.bucket());

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
                                "XML is invalid.");

    auto bucket_id = req.bucket();
    std::vector<std::string> success;
    std::vector<fail> failure;
    for (const auto& obj : object_nodes) {
        auto key = obj.get().get_optional<std::string>("Key");
        if (!key) {
            throw command_exception(status::bad_request, "MalformedXML",
                                    "XML is invalid.");
        }

        try {
            LOG_DEBUG() << req.peer() << ": delete_objects::handle(): deleting "
                        << *key;

            co_await m_dir.delete_object(req.bucket(), *key);
            success.emplace_back(*key);

        } catch (const std::exception& e) {
            failure.emplace_back(*key, e.what());
        }
    }

    co_return get_response(success, failure);
}

std::string delete_objects::action_id() const { return "s3:DeleteObjects"; }

} // namespace uh::cluster
