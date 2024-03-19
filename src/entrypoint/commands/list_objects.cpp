#include "list_objects.h"
#include "common/utils/strings.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

struct collapsed_objects {
    std::optional<std::string> prefix{};
    std::optional<std::reference_wrapper<const object>> object{};
};

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
            if (auto& val = req_uri.get_query_string_value(key); !val.empty())
                return std::make_optional<std::string>(val);
        }
        return std::nullopt;
    };

    const auto marker = get_if_exists("marker");
    const auto prefix = get_if_exists("prefix");
    const auto delimiter = get_if_exists("delimiter");
    const auto encoding_type = get_if_exists("encoding-type");
    const auto max_keys_val = get_if_exists("max-keys");

    size_t max_keys = 1000;
    if (max_keys_val) {
        max_keys = std::stoul(*max_keys_val);
    }

    bool common_prefix_last;
    std::vector<collapsed_objects> collapsed_objs;

    if (!objects.empty() && max_keys > 0) {

        for (std::string previous; const auto& object : objects) {
            size_t delimiter_index = std::string::npos;

            if (delimiter) {
                if (prefix) {
                    delimiter_index =
                        object.name.find(*delimiter, prefix->size());
                } else {
                    delimiter_index = object.name.find(*delimiter);
                }
            }

            if (delimiter_index != std::string::npos) {
                auto delimiter_prefix =
                    object.name.substr(0, delimiter_index + 1);
                if (previous != delimiter_prefix) {
                    collapsed_objs.emplace_back(delimiter_prefix, std::nullopt);
                    previous = delimiter_prefix;
                }
            } else {
                collapsed_objs.emplace_back(std::nullopt, std::cref(object));
            }
        }

        std::string common_prefixes_xml_string;
        std::string next_marker_xml;

        std::string is_truncated = "false";
        std::string contents_xml;

        size_t contents_counter = 0;
        size_t common_prefixes_counter = 0;

        for (const auto& object : collapsed_objs) {
            if (object.prefix) {
                common_prefixes_xml_string += "<CommonPrefixes>\n<Prefix>" +
                                              *object.prefix +
                                              "</Prefix>\n</CommonPrefixes>\n";
                common_prefix_last = true;
                ++common_prefixes_counter;
            } else if (object.object) {
                contents_xml +=
                    "<Contents>\n"
                    "<LastModified>" +
                    object.object->get().last_modified +
                    "</LastModified>\n"
                    "<Key>" +
                    (encoding_type ? url_encode(object.object->get().name)
                                   : object.object->get().name) +
                    "</Key>\n"
                    "<Size>" +
                    std::to_string(object.object->get().size) +
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
                            "<NextMarker>" + *object.prefix + "</NextMarker>";
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
        delimiter_xml_string =
            "<Delimiter>" +
            (encoding_type ? url_encode(*delimiter) : *delimiter) +
            "</Delimiter>\n";
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
        prefix_xml = "<Prefix>" +
                     (encoding_type ? url_encode(*prefix) : *prefix) +
                     "</Prefix>\n";
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

        directory_list_objects_message list_objs_res;
        auto func = [](const directory_message& dir_req,
                       directory_list_objects_message& list_objs_res,
                       client::acquired_messenger m) -> coro<void> {
            co_await m.get().send_directory_message(DIRECTORY_OBJECT_LIST_REQ,
                                                    dir_req);
            const auto h_dir = co_await m.get().recv_header();

            unique_buffer<char> buffer(h_dir.size);

            list_objs_res =
                co_await m.get().recv_directory_list_objects_message(h_dir);
        };

        co_await m_collection.workers
            .io_thread_acquire_messenger_and_post_in_io_threads(
                m_collection.directory_services.get(),
                std::bind_front(func, std::cref(dir_req),
                                std::ref(list_objs_res)));

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
