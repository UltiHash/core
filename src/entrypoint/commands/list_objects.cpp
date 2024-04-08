#include "list_objects.h"
#include "common/utils/strings.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

namespace http = boost::beast::http;

list_objects::list_objects(const reference_collection& collection)
    : m_collection(collection) {}

bool list_objects::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() &&
           !uri.query_string_exists("uploads") &&
           !uri.query_string_exists("list-type");
}

static http_response get_response(const std::vector<object>& objects,
                                  const http_request& req) {

    const auto& req_uri = req.get_uri();

    const auto get_if_exists =
        [&req_uri](auto&& key) -> std::optional<std::string> {
        if (req_uri.query_string_exists(key)) {
            return req_uri.get_query_string_value(key);
        }
        return std::nullopt;
    };

    const auto marker = get_if_exists("marker");
    const auto prefix = get_if_exists("prefix");

    std::optional<std::string> delimiter = std::nullopt;
    if (req_uri.query_string_exists("delimiter")) {
        if (auto value = req_uri.get_query_string_value("delimiter");
            !value.empty())
            delimiter = value;
    }

    const auto encoding_type = get_if_exists("encoding-type");
    if (encoding_type) {
        if (*encoding_type != "url") {
            throw command_exception(http::status::bad_request,
                                    command_error::invalid_query_parameter);
        }
    }

    size_t max_keys = 1000;
    const auto max_keys_val = get_if_exists("max-keys");
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
                    object._object->get().last_modified +
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
    name_xml_string += "<Name>" + req_uri.get_bucket_id() + "</Name>\n";

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
        const auto& req_uri = req.get_uri();

        directory_message dir_req;
        dir_req.bucket_id = req_uri.get_bucket_id();

        if (req_uri.query_string_exists("prefix")) {
            if (const auto& prefix = req_uri.get_query_string_value("prefix");
                !prefix.empty()) {
                dir_req.object_key_prefix =
                    std::make_unique<std::string>(prefix);
            }
        }

        if (req_uri.query_string_exists("marker")) {
            if (const auto& marker = req_uri.get_query_string_value("marker");
                !marker.empty()) {
                dir_req.object_key_lower_bound =
                    std::make_unique<std::string>(marker);
            }
        }

        auto cl = m_collection.directory_services.get();
        auto mgr = co_await cl->acquire_messenger();

        co_await mgr->send_directory_message(DIRECTORY_OBJECT_LIST_REQ,
                                             dir_req);

        auto h_dir = co_await mgr->recv_header();
        auto list_objs_res =
            co_await mgr->recv_directory_list_objects_message(h_dir);

        auto res = get_response(list_objs_res.objects, req);
        co_await req.respond(res.get_prepared_response());

    } catch (const error_exception& e) {
        LOG_ERROR() << e.what();
        switch (*e.error()) {
        case error::bucket_not_found:
            throw command_exception(http::status::not_found,
                                    command_error::bucket_not_found);
        case error::invalid_bucket_name:
            throw command_exception(http::status::bad_request,
                                    command_error::invalid_bucket_name);
        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
