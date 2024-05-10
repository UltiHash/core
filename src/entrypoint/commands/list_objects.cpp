#include "list_objects.h"
#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

namespace http = boost::beast::http;

list_objects::list_objects(const reference_collection& collection)
    : m_collection(collection) {}

bool list_objects::can_handle(const http_request& req) {
    return req.method() == method::get && !req.bucket().empty() &&
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

    std::string common_prefixes_xml_string;
    std::string contents_xml;
    std::string next_marker_xml;
    std::string is_truncated = "false";

    if (!objects.empty() && max_keys > 0) {

        bool common_prefix_last = false;
        size_t contents_counter = 0;
        size_t common_prefixes_counter = 0;

        auto collapsed_objs = retrieval::collapse(objects, delimiter, prefix);

        for (const auto& object : collapsed_objs) {
            if (object._prefix) {
                common_prefixes_xml_string +=
                    "<CommonPrefixes>\n<Prefix>" +
                    (encoding_type ? url_encode(*object._prefix)
                                   : *object._prefix) +
                    "</Prefix>\n</CommonPrefixes>\n";
                common_prefix_last = true;
                ++common_prefixes_counter;
            } else if (object._object) {
                contents_xml +=
                    "<Contents>\n"
                    "<LastModified>" +
                    iso8601_date(object._object->get().last_modified) +
                    "</LastModified>\n"
                    "<Key>" +
                    (encoding_type ? url_encode(object._object->get().name)
                                   : object._object->get().name) +
                    "</Key>\n"
                    "<Size>" +
                    std::to_string(object._object->get().size) +
                    "</Size>\n"
                    "</Contents>\n";
                common_prefix_last = false;
                ++contents_counter;
            }

            if (contents_counter + common_prefixes_counter == max_keys &&
                collapsed_objs.size() > max_keys) {
                is_truncated = "true";
                if (delimiter) {
                    if (common_prefix_last)
                        next_marker_xml =
                            "<NextMarker>" + *object._prefix + "</NextMarker>";
                    else
                        next_marker_xml = "<NextMarker>" +
                                          objects[max_keys - 1].name +
                                          "</NextMarker>";
                }
                break;
            }
        }
    }

    std::string delimiter_xml_string;
    if (delimiter) {
        delimiter_xml_string = "<Delimiter>" + *delimiter + "</Delimiter>\n";
    }

    std::string name_xml_string;
    name_xml_string += "<Name>" + req.bucket() + "</Name>\n";

    std::string max_keys_xml_string =
        "<MaxKeys>" + std::to_string(max_keys) + "</MaxKeys>\n";

    std::string encoding_type_xml;
    if (encoding_type) {
        encoding_type_xml =
            "<EncodingType>" + *encoding_type + "</EncodingType>\n";
    }

    std::string prefix_xml;
    if (prefix) {
        prefix_xml = "<Prefix>" + *prefix + "</Prefix>\n";
    }

    std::string marker_xml;
    if (marker) {
        marker_xml = "<Marker>" + *marker + "</Marker>\n";
    }

    http_response res;
    res.set_body(std::string("<ListBucketResult>\n"
                             "<IsTruncated>" +
                             is_truncated + "</IsTruncated>\n" + marker_xml +
                             next_marker_xml + contents_xml + name_xml_string +
                             prefix_xml + delimiter_xml_string +
                             max_keys_xml_string + common_prefixes_xml_string +
                             encoding_type_xml + "</ListBucketResult>"));

    return res;
}

coro<void> list_objects::handle(http_request& req) const {
    metric<entrypoint_list_objects_req>::increase(1);
    try {
        auto obj_list = co_await m_collection.directory.list_objects(
            req.bucket(), req.query("prefix"), req.query("marker"));

        auto res = get_response(obj_list, req);
        co_await req.respond(res.get_prepared_response());

    } catch (const error_exception& e) {
        LOG_ERROR() << e.what();
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
