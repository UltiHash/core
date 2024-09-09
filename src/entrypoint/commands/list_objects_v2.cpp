#include "list_objects_v2.h"
#include <boost/property_tree/ptree.hpp>

#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

namespace {

std::string ident(const std::string& s) noexcept { return s; }

auto get_encoder(std::optional<std::string> encoding_type) {
    if (!encoding_type) {
        return ident;
    }

    if (*encoding_type != "url") {
        throw command_exception(http::status::bad_request,
                                "InvalidQueryParameters",
                                "encountered unexpected query parameter");
    }

    return url_encode;
}

http_response get_response(const std::vector<object>& objects,
                           const http_request& req) {

    const auto prefix = req.query("prefix");

    auto delimiter = req.query("delimiter");
    if (delimiter && delimiter->empty()) {
        delimiter = std::nullopt;
    }

    auto encoding_type = req.query("encoding-type");
    auto encode = get_encoder(encoding_type);
    auto max_keys = query<std::size_t>(req, "max-keys").value_or(1000);
    auto fetch_owner_set = query<bool>(req, "fetch-owner").value_or(false);

    std::string is_truncated = "false";
    boost::property_tree::ptree pt;
    boost::property_tree::ptree result_node;
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
                auto& node = common_prefixes_nodes.emplace_back();

                put(node, "Prefix", encode(*object._prefix));
                common_prefix_last = true;
                ++common_prefixes_counter;
            } else if (object._object) {
                auto& node = contents_nodes.emplace_back();
                put(node, "ETag", object._object->get().etag);
                put(node, "Key", encode(object._object->get().name));
                put(node, "Size", object._object->get().size);
                put(node, "LastModified",
                    iso8601_date(object._object->get().last_modified));
                if (fetch_owner_set) {
                    put(node, "Owner", "no-owner-support");
                }

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

    put(result_node, "<xmlattr>.xmlns",
        "http://s3.amazonaws.com/doc/2006-03-01/");
    put(result_node, "IsTruncated", is_truncated);
    put(result_node, "Name", req.bucket());
    put(result_node, "Prefix", prefix);
    put(result_node, "Delimiter", delimiter);
    put(result_node, "MaxKeys", max_keys);
    put(result_node, "EncodingType", encoding_type);
    put(result_node, "KeyCount", contents_counter + common_prefixes_counter);
    put(result_node, "ContinuationToken", query(req, "continuation-token"));
    put(result_node, "NextContinuationToken", next_continuation_token);
    put(result_node, "StartAfter", req.query("start-after"));

    for (const auto& contents : contents_nodes) {
        result_node.add_child("Contents", contents);
    }

    for (const auto& common_prefixes : common_prefixes_nodes) {
        result_node.add_child("CommonPrefixes", common_prefixes);
    }

    pt.add_child("ListBucketResult", result_node);

    http_response res;
    res << pt;

    return res;
}

} // namespace

list_objects_v2::list_objects_v2(directory& dir)
    : m_directory(dir) {}

bool list_objects_v2::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           req.object_key().empty() && req.query("list-type") &&
           *req.query("list-type") == "2";
}

coro<http_response> list_objects_v2::handle(http_request& req) {
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
        obj_list =
            co_await m_directory.list_objects(req.bucket(), prefix, lowerbound);
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    co_return get_response(obj_list, req);
}

std::string list_objects_v2::action_id() const { return "s3:ListObjectsV2"; }

} // namespace uh::cluster
