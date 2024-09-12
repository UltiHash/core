#include "list_objects.h"
#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/utils.h"
#include <boost/property_tree/ptree.hpp>

namespace http = boost::beast::http;
using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

std::string ident(const std::string& s) noexcept { return s; }

auto get_encoder(std::optional<std::string> encoding_type) {
    if (!encoding_type) {
        return ident;
    }

    if (*encoding_type != "url") {
        throw command_exception(status::bad_request, "InvalidQueryParameters",
                                "encountered unexpected query parameter");
    }

    return url_encode;
}

response get_response(const std::vector<object>& objects, const request& req) {

    const auto prefix = req.query("prefix");

    std::optional<std::string> delimiter = req.query("delimiter");
    if (delimiter && delimiter->empty()) {
        delimiter = std::nullopt;
    }

    auto encoding_type = req.query("encoding-type");
    auto encode = get_encoder(encoding_type);

    std::size_t max_keys = query<std::size_t>(req, "max-keys").value_or(1000);

    std::string is_truncated = "false";
    boost::property_tree::ptree pt;
    boost::property_tree::ptree result_node;
    std::vector<boost::property_tree::ptree> contents_nodes;
    std::vector<boost::property_tree::ptree> common_prefixes_nodes;
    std::optional<std::string> next_marker;

    if (!objects.empty() && max_keys > 0) {

        bool common_prefix_last = false;
        size_t contents_counter = 0;
        size_t common_prefixes_counter = 0;

        auto collapsed_objs = retrieval::collapse(objects, delimiter, prefix);

        for (const auto& object : collapsed_objs) {
            if (object._prefix) {
                auto& node = common_prefixes_nodes.emplace_back();
                put(node, "Prefix", encode(*object._prefix));
                common_prefix_last = true;
                ++common_prefixes_counter;
            } else if (object._object) {
                boost::property_tree::ptree& node =
                    contents_nodes.emplace_back();

                put(node, "ETag", object._object->get().etag);
                put(node, "Key", encode(object._object->get().name));
                put(node, "Size", object._object->get().size);
                put(node, "LastModified",
                    iso8601_date(object._object->get().last_modified));

                common_prefix_last = false;
                ++contents_counter;
            }

            if (contents_counter + common_prefixes_counter == max_keys &&
                collapsed_objs.size() > max_keys) {
                is_truncated = "true";
                if (delimiter) {
                    if (common_prefix_last)
                        next_marker = *object._prefix;
                    else
                        next_marker = objects[max_keys - 1].name;
                }
                break;
            }
        }
    }

    put(result_node, "IsTruncated", is_truncated);
    put(result_node, "Marker", req.query("marker"));
    put(result_node, "NextMarker", next_marker);
    put(result_node, "Name", req.bucket());
    put(result_node, "Prefix", prefix);
    put(result_node, "Delimiter", delimiter);
    put(result_node, "MaxKeys", max_keys);
    put(result_node, "EncodingType", encoding_type);

    for (const auto& contents : contents_nodes) {
        result_node.add_child("Contents", contents);
    }

    for (const auto& common_prefixes : common_prefixes_nodes) {
        result_node.add_child("CommonPrefixes", common_prefixes);
    }

    pt.add_child("ListBucketResult", result_node);

    response res;
    res << pt;

    return res;
}

} // namespace

list_objects::list_objects(directory& dir)
    : m_directory(dir) {}

bool list_objects::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && req.object_key().empty() &&
           !req.query("uploads") && !req.query("list-type");
}

coro<response> list_objects::handle(request& req) {
    metric<entrypoint_list_objects_req>::increase(1);

    std::vector<object> obj_list;
    try {
        obj_list = co_await m_directory.list_objects(
            req.bucket(), req.query("prefix"), req.query("marker"));
    } catch (const std::exception& e) {
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    co_return get_response(obj_list, req);
}

std::string list_objects::action_id() const { return "s3:ListObjects"; }

} // namespace uh::cluster
