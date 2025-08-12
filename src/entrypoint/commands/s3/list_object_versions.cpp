#include "list_object_versions.h"

#include <entrypoint/formats.h>
#include <entrypoint/utils.h>

#include <boost/property_tree/xml_parser.hpp>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

list_object_versions::list_object_versions(directory& dir)
    : m_dir(dir) {}

bool list_object_versions::can_handle(const ep::http::request& req) {
    return req.method() == verb::get && req.bucket() != RESERVED_BUCKET_NAME &&
        !req.bucket().empty() && req.object_key().empty() &&
        req.query("versions") && !req.query("versioning");
}

coro<ep::http::response> list_object_versions::handle(ep::http::request& req) {
    auto encoding_type = req.query("encoding-type");
    auto encode = encoder(encoding_type);

    auto delimiter = req.query("delimiter");
    auto max_keys = query<std::size_t>(req, "max-keys").value_or(1000);

    auto key_marker = req.query("key-marker");
    auto version_id_marker = req.query("version-id-marker");
    auto prefix = req.query("prefix");

    auto objects = co_await m_dir.list_object_versions(req.bucket(), prefix, key_marker, version_id_marker, max_keys + 1);

    boost::property_tree::ptree result_node;
    put(result_node, "<xmlattr>.xmlns",
        "http://s3.amazonaws.com/doc/2006-03-01/");
    put(result_node, "IsTruncated", objects.size() > max_keys);
    put(result_node, "KeyMarker", encode(key_marker));
    put(result_node, "VersionMarker", version_id_marker);
    put(result_node, "Name", req.bucket());
    put(result_node, "Prefix", encode(prefix));
    put(result_node, "Delimiter", encode(delimiter));
    put(result_node, "MaxKeys", max_keys);
    put(result_node, "EncodingType", encoding_type);

    if (objects.size() > max_keys) {
        put(result_node, "NextKeyMarker", encode(objects.back().name));
        put(result_node, "NextVersionMarker", objects.back().version);
    }

    std::set<std::string> prefixes;
    std::set<std::string> key_seen;
    std::size_t prefix_length = prefix ? prefix->size() : 0ull;

    for (const auto& obj : objects) {

        if (delimiter) {
            auto first = obj.name.find(*delimiter, prefix_length);
            if (first != std::string::npos) {
                prefixes.insert(obj.name.substr(0, first));
                continue;
            }
        }

        auto latest = key_seen.insert(obj.name).second;

        if (obj.state == ep::object_state::normal) {
            boost::property_tree::ptree node;

            put(node, "ETag", obj.etag);
            put(node, "Key", encode(obj.name));
            put(node, "Size", obj.size);
            put(node, "IsLatest", latest);
            put(node, "LastModified", iso8601_date(obj.last_modified));
            put(node, "VersionId", obj.version);

            result_node.add_child("Version", node);
        } else {
            boost::property_tree::ptree node;

            put(node, "Key", encode(obj.name));
            put(node, "IsLatest", latest);
            put(node, "LastModified", iso8601_date(obj.last_modified));
            put(node, "VersionId", obj.version);

            result_node.add_child("DeleteMarker", node);
        }
    }

    if (!prefixes.empty()) {
        boost::property_tree::ptree prefixes_node;
        for (const auto& pfx : prefixes) {
            boost::property_tree::ptree node(*encode(pfx));
            prefixes_node.add_child("Prefix", node);
        }

        result_node.add_child("CommonPrefixes", prefixes_node);
    }

    boost::property_tree::ptree pt;
    pt.add_child("ListVersionsResult", result_node);

    response res;
    res << pt;

    co_return res;
}

std::string list_object_versions::action_id() const {
    return "s3:ListBucketVersions"; // sic!
}

} // namespace uh::cluster
