#include "list_objects_v2.h"

#include "common/utils/strings.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_response.h"

namespace uh::cluster {

namespace {

http_response get_response(const std::vector<object>& objects,
                           const http_request& req) {

    const auto& req_uri = req.get_uri();

    const auto get_if_exists =
        [&req_uri](auto&& key) -> std::optional<std::string> {
        if (req_uri.query_string_exists(key)) {
            return req_uri.get_query_string_value(key);
        }
        return std::nullopt;
    };

    const auto start_after = get_if_exists("start-after");
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

    const auto continuation_token = get_if_exists("continuation-token");

    size_t max_keys = 1000;
    if (const auto max_keys_str = get_if_exists("max-keys");
        max_keys_str.has_value()) {
        max_keys = std::stoul(*max_keys_str);
    }

    bool fetch_owner_set = false;
    if (const auto fetch_owner = get_if_exists("fetch-owner");
        fetch_owner.has_value()) {
        if (to_bool(*fetch_owner))
            fetch_owner_set = true;
    }

    std::string common_prefixes_xml_string;
    std::string content_xml_string;
    std::string is_truncated = "false";
    std::string next_continuation_token_xml;
    size_t contents_counter = 0;
    size_t common_prefixes_counter = 0;

    if (!objects.empty() && max_keys != 0) {
        bool common_prefix_last = false;

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
                content_xml_string +=
                    "<Contents>\n"
                    "<LastModified>" +
                    object._object->get().last_modified +
                    "</LastModified>\n"
                    "<Key>" +
                    (encoding_type ? url_encode(object._object->get().name)
                                   : object._object->get().name) +
                    "</Key>\n" +
                    (fetch_owner_set ? "<Owner>no-owner-support</Owner>" : "") +
                    "<Size>" + std::to_string(object._object->get().size) +
                    +"</Size>\n"
                     "</Contents>\n";
                common_prefix_last = false;
                ++contents_counter;
            }

            if (contents_counter + common_prefixes_counter == max_keys &&
                collapsed_objs.size() > max_keys) {
                is_truncated = "true";
                if (common_prefix_last)
                    next_continuation_token_xml = "<NextContinuationToken>" +
                                                  *object._prefix +
                                                  "</NextContinuationToken>";
                else
                    next_continuation_token_xml = "<NextContinuationToken>" +
                                                  objects[max_keys - 1].name +
                                                  "</NextContinuationToken>";
                break;
            }
        }
    }

    std::string delimiter_xml_string;
    if (delimiter) {
        delimiter_xml_string = "<Delimiter>" + *delimiter + "</Delimiter>\n";
    }

    std::string key_count_xml;
    content_xml_string +=
        "<KeyCount>" +
        std::to_string(contents_counter + common_prefixes_counter) +
        "</KeyCount>\n";

    std::string max_keys_xml_string =
        "<MaxKeys>" + std::to_string(max_keys) + "</MaxKeys>\n";

    std::string encoding_type_xml_string;
    if (encoding_type) {
        encoding_type_xml_string =
            "<EncodingType>" + *encoding_type + "</EncodingType>\n";
    }

    std::string continuation_token_xml;
    ;
    if (continuation_token) {
        continuation_token_xml = "<ContinuationToken>" + *continuation_token +
                                 "</ContinuationToken>\n";
    }

    std::string start_after_xml_string;
    if (start_after) {
        encoding_type_xml_string =
            "<StartAfter>" + *start_after + "</StartAfter>\n";
    }

    std::string prefix_xml_string;
    if (prefix) {
        prefix_xml_string = "<Prefix>" + *prefix + "</Prefix>\n";
    }

    http_response res;
    res.set_body("<ListBucketResult>\n"
                 "<IsTruncated>" +
                 is_truncated + "</IsTruncated>\n" + content_xml_string +
                 prefix_xml_string + delimiter_xml_string +
                 max_keys_xml_string + common_prefixes_xml_string +
                 encoding_type_xml_string + key_count_xml +
                 continuation_token_xml + start_after_xml_string +
                 next_continuation_token_xml + "</ListBucketResult>");

    return res;
}

} // namespace

list_objects_v2::list_objects_v2(const reference_collection& collection)
    : m_collection(collection) {}

bool list_objects_v2::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() &&
           uri.query_string_exists("list-type") &&
           uri.get_query_string_value("list-type") == "2";
}

coro<void> list_objects_v2::handle(http_request& req) const {
    metric<entrypoint_list_objects_v2_req>::increase(1);
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

        if (req_uri.query_string_exists("start-after")) {
            if (const auto& start_after =
                    req_uri.get_query_string_value("start-after");
                !start_after.empty()) {
                dir_req.object_key_lower_bound =
                    std::make_unique<std::string>(start_after);
            }
        }
        if (req_uri.query_string_exists("continuation-token")) {
            if (const auto& continuation_token =
                    req_uri.get_query_string_value("continuation-token");
                !continuation_token.empty()) {
                if (!dir_req.object_key_lower_bound ||
                    continuation_token > *dir_req.object_key_lower_bound)
                    dir_req.object_key_lower_bound =
                        std::make_unique<std::string>(continuation_token);
            }
        }

        directory_list_objects_message list_objs_res;

        auto client = m_collection.directory_services.get();
        auto m = co_await client->acquire_messenger();

        co_await m->send_directory_message(DIRECTORY_OBJECT_LIST_REQ, dir_req);
        const auto h_dir = co_await m->recv_header();

        list_objs_res = co_await m->recv_directory_list_objects_message(h_dir);

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
