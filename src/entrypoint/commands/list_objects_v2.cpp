#include "list_objects_v2.h"

#include "common/utils/strings.h"
#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_response.h"

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
                    iso8601_date(object._object->get().last_modified) +
                    "</LastModified>\n";

                if (object._object->get().etag) {
                    content_xml_string +=
                        "<ETag>" + *object._object->get().etag + "</ETag>\n";
                }

                content_xml_string +=
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
    return req.method() == method::get && !req.bucket().empty() &&
           req.object_key().empty() && req.query("list-type") &&
           *req.query("list-type") == "2";
}

coro<void> list_objects_v2::handle(http_request& req) const {
    metric<entrypoint_list_objects_v2_req>::increase(1);
    try {
        std::optional<std::string> prefix = req.query("prefix");
        std::optional<std::string> lowerbound = req.query("start-after");

        if (auto continuation_token = req.query("continuation-token");
            continuation_token && !continuation_token->empty()) {
            if (!lowerbound || *continuation_token > *lowerbound)
                lowerbound = continuation_token;
        }

        auto obj_list = co_await m_collection.directory.list_objects(
            req.bucket(), prefix, lowerbound);

        auto res = get_response(obj_list, req);

        LOG_DEBUG() << req.socket().remote_endpoint()
                    << " list_objects_v2 response: " << res;
        co_await req.respond(res.get_prepared_response());

    } catch (const error_exception& e) {
        LOG_ERROR() << req.socket().remote_endpoint() << ": " << e.what();
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
