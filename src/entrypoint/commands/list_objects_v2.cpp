#include "list_objects_v2.h"
#include <boost/property_tree/ptree.hpp>

#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

namespace {

http_response get_response(const std::vector<object>& objects,
                           const http_request& req) {

    const auto start_after = req.query("start-after");
    const auto prefix = req.query("prefix");

    auto delimiter = req.query("delimiter");
    if (delimiter && delimiter->empty()) {
        delimiter = std::nullopt;
    }

    const auto encoding_type = req.query("encoding-type");
    if (encoding_type && *encoding_type != "url") {
        throw command_exception(http::status::bad_request,
                                "InvalidQueryParameters",
                                "encountered unexpected query parameter");
    }

    const auto continuation_token = req.query("continuation-token");

    size_t max_keys = 1000;
    if (const auto max_keys_str = req.query("max-keys");
        max_keys_str.has_value()) {
        max_keys = std::stoul(*max_keys_str);
    }

    bool fetch_owner_set = false;
    if (const auto fetch_owner = req.query("fetch-owner");
        fetch_owner.has_value()) {
        if (to_bool(*fetch_owner))
            fetch_owner_set = true;
    }

    std::string is_truncated = "false";
    boost::property_tree::ptree pt;
    boost::property_tree::ptree list_bucket_result_node;
    std::vector<boost::property_tree::ptree> contents_nodes;
    std::vector<boost::property_tree::ptree> common_prefixes_nodes;
    std::optional<std::string> next_continuation_token;

    size_t contents_counter = 0;
    size_t common_prefixes_counter = 0;

    if (!objects.empty() && max_keys != 0) {
        bool common_prefix_last = false;

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
                if (fetch_owner_set) {
                    contents_node.put("Owner", "no-owner-support");
                }
                contents_node.put("Size", object._object->get().size);

                common_prefix_last = false;
                ++contents_counter;
            }

            if (contents_counter + common_prefixes_counter == max_keys &&
                collapsed_objs.size() > max_keys) {
                is_truncated = "true";
                if (common_prefix_last)
                    next_continuation_token = *object._prefix;
                else
                    next_continuation_token = objects[max_keys - 1].name;
                break;
            }
        }
    }

    list_bucket_result_node.put("<xmlattr>.xmlns",
                                "http://s3.amazonaws.com/doc/2006-03-01/");
    list_bucket_result_node.put("IsTruncated", is_truncated);

    for (const auto& contents : contents_nodes) {
        list_bucket_result_node.add_child("Contents", contents);
    }

    list_bucket_result_node.put("Name", req.bucket());

    if (prefix) {
        list_bucket_result_node.put("Prefix", *prefix);
    }

    if (delimiter) {
        list_bucket_result_node.put("Delimiter", *delimiter);
    }

    list_bucket_result_node.put("MaxKeys", max_keys);

    for (const auto& common_prefixes : common_prefixes_nodes) {
        list_bucket_result_node.add_child("CommonPrefixes", common_prefixes);
    }

    if (encoding_type) {
        list_bucket_result_node.put("EncodingType", *encoding_type);
    }

    list_bucket_result_node.put("KeyCount",
                                contents_counter + common_prefixes_counter);

    if (continuation_token)
        list_bucket_result_node.put("ContinuationToken", *continuation_token);

    if (next_continuation_token)
        list_bucket_result_node.put("NextContinuationToken",
                                    *next_continuation_token);

    if (start_after)
        list_bucket_result_node.put("StartAfter", *start_after);

    pt.add_child("ListBucketResult", list_bucket_result_node);

    http_response res;
    res << pt;

    return res;
}

} // namespace

list_objects_v2::list_objects_v2(const reference_collection& collection)
    : m_collection(collection) {}

bool list_objects_v2::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && req.query("list-type") &&
           *req.query("list-type") == "2";
}

coro<http_response> list_objects_v2::handle(http_request& req) const {
    metric<entrypoint_list_objects_v2_req>::increase(1);
    std::optional<std::string> prefix = req.query("prefix");
    std::optional<std::string> lowerbound = req.query("start-after");

    if (auto continuation_token = req.query("continuation-token");
        continuation_token && !continuation_token->empty()) {
        if (!lowerbound || *continuation_token > *lowerbound)
            lowerbound = continuation_token;
    }

    std::vector<object> obj_list;
    try {
        obj_list = co_await m_collection.directory.list_objects(
            req.bucket(), prefix, lowerbound);
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    co_return get_response(obj_list, req);
}

} // namespace uh::cluster
