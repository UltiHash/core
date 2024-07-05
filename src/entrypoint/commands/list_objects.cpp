#include "list_objects.h"
#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"
#include <boost/property_tree/ptree.hpp>

namespace uh::cluster {

namespace http = boost::beast::http;

list_objects::list_objects(const reference_collection& collection)
    : m_collection(collection) {}

bool list_objects::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && !req.query("uploads") &&
           !req.query("list-type");
}

static http_response get_response(const std::vector<object>& objects,
                                  const http_request& req) {

    const auto marker = req.query("marker");
    const auto prefix = req.query("prefix");

    std::optional<std::string> delimiter = req.query("delimiter");
    if (delimiter && delimiter->empty()) {
        delimiter = std::nullopt;
    }

    const auto encoding_type = req.query("encoding-type");
    if (encoding_type && *encoding_type != "url") {
        throw command_exception(http::status::bad_request,
                                "InvalidQueryParameters",
                                "encountered unexpected query parameter");
    }

    size_t max_keys = 1000;
    const auto max_keys_val = req.query("max-keys");
    if (max_keys_val) {
        max_keys = std::stoul(*max_keys_val);
    }

    std::string is_truncated = "false";
    boost::property_tree::ptree pt;
    boost::property_tree::ptree list_bucket_result_node;
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
                boost::property_tree::ptree& common_prefixes_node =
                    common_prefixes_nodes.emplace_back();
                if (encoding_type) {
                    common_prefixes_node.put("Prefix",
                                             url_encode(*object._prefix));
                } else {
                    common_prefixes_node.put("Prefix", *object._prefix);
                }
                common_prefix_last = true;
                ++common_prefixes_counter;
            } else if (object._object) {
                boost::property_tree::ptree& contents_node =
                    contents_nodes.emplace_back();
                if (object._object->get().etag) {
                    contents_node.put("ETag", *object._object->get().etag);
                }

                if (encoding_type) {
                    contents_node.put("Key",
                                      url_encode(object._object->get().name));
                } else {
                    contents_node.put("Key", object._object->get().name);
                }

                contents_node.put(
                    "LastModified",
                    iso8601_date(object._object->get().last_modified));
                contents_node.put("Size", object._object->get().size);

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

    list_bucket_result_node.put("IsTruncated", is_truncated);

    if (marker)
        list_bucket_result_node.put("Marker", *marker);

    if (next_marker)
        list_bucket_result_node.put("NextMarker", *next_marker);

    for (const auto& contents : contents_nodes) {
        list_bucket_result_node.add_child("Contents", contents);
    }

    list_bucket_result_node.put("Name", req.bucket());

    if (prefix)
        list_bucket_result_node.put("Prefix", *prefix);

    if (delimiter)
        list_bucket_result_node.put("Delimiter", *delimiter);

    list_bucket_result_node.put("MaxKeys", max_keys);

    for (const auto& common_prefixes : common_prefixes_nodes) {
        list_bucket_result_node.add_child("CommonPrefixes", common_prefixes);
    }

    if (encoding_type)
        list_bucket_result_node.put("EncodingType", *encoding_type);

    pt.add_child("ListBucketResult", list_bucket_result_node);

    http_response res;
    res << pt;

    return res;
}

coro<void> list_objects::handle(http_request& req) const {
    metric<entrypoint_list_objects_req>::increase(1);

    std::vector<object> obj_list;
    try {
        obj_list = co_await m_collection.directory.list_objects(
            req.bucket(), req.query("prefix"), req.query("marker"));
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    auto res = get_response(obj_list, req);
    co_await req.respond(res.get_prepared_response());
}

} // namespace uh::cluster
