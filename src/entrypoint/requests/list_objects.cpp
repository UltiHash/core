#include "list_objects.h"
#include "common/utils/strings.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/custom_error_response_exception.h"

namespace uh::cluster {

list_objects::list_objects(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool list_objects::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() &&
           !uri.query_string_exists("uploads") &&
           !uri.query_string_exists("list-type");
}

static http_response get_response(const std::vector<std::string>& contents,
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

    std::set<std::string> common_prefixes;
    std::string contents_xml;

    std::size_t till_marker_count = 0;
    size_t counter = 0;

    if (!contents.empty() && max_keys != 0) {

        auto content_itr = contents.begin();
        if (marker) {
            auto index_itr = find_lexically_closest(contents, *marker);

            if (index_itr != contents.end()) {
                content_itr = index_itr++;
            }
        }

        till_marker_count = content_itr - contents.begin();

        for (const auto& c : contents) {
            size_t delimiter_index = std::string::npos;

            if (delimiter) {
                if (prefix) {
                    delimiter_index = c.find(*delimiter, prefix->size());
                } else {
                    delimiter_index = c.find(*delimiter);
                }
            }
            if (delimiter && delimiter_index != std::string::npos) {
                auto delimiter_prefix = c.substr(0, delimiter_index + 1);
                common_prefixes.emplace((encoding_type
                                             ? url_encode(delimiter_prefix)
                                             : delimiter_prefix));
            } else {
                contents_xml += "<Contents>\n"
                                "<Key>" +
                                (encoding_type ? url_encode(c) : c) +
                                "</Key>\n" + "</Contents>\n";
                counter++;
            }

            if (counter + common_prefixes.size() == max_keys)
                break;
        }
    }

    std::string common_prefixes_xml_string;
    for (const auto& common_prefix : common_prefixes) {
        common_prefixes_xml_string += "<CommonPrefixes>\n<Prefix>" +
                                      common_prefix +
                                      "</Prefix>\n</CommonPrefixes>\n";
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

    std::string is_truncated = "false";
    std::string next_marker_xml;
    if ((contents.size() - till_marker_count > max_keys) && max_keys != 0) {
        is_truncated = "true";
        if (delimiter)
            next_marker_xml = "<NextMarker>" +
                              contents[till_marker_count + max_keys] +
                              "</NextMarker>";
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

coro<http_response> list_objects::handle(const http_request& req) const {

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

        std::vector<std::string> contents;
        auto func = [](const directory_message& dir_req,
                       std::vector<std::string>& contents,
                       client::acquired_messenger m) -> coro<void> {
            co_await m.get().send_directory_message(DIRECTORY_OBJECT_LIST_REQ,
                                                    dir_req);
            const auto h_dir = co_await m.get().recv_header();

            unique_buffer<char> buffer(h_dir.size);
            directory_lst_entities_message list_objects_res;

            list_objects_res =
                co_await m.get().recv_directory_list_entities_message(h_dir);

            for (const auto& content : list_objects_res.entities) {
                contents.emplace_back(content);
            }
        };

        co_await worker_utils::
            io_thread_acquire_messenger_and_post_in_io_threads(
                m_state.workers, m_state.ioc, m_state.directory_services.get(),
                std::bind_front(func, std::cref(dir_req), std::ref(contents)));

        co_return get_response(contents, req);
    } catch (const error_exception& e) {
        LOG_ERROR() << e.what();
        switch (*e.error()) {
        case error::bucket_not_found:
            throw custom_error_response_exception(
                boost::beast::http::status::not_found,
                http_error::bucket_not_found);
        case error::invalid_bucket_name:
            throw custom_error_response_exception(
                boost::beast::http::status::bad_request,
                http_error::invalid_bucket_name);
        default:
            throw custom_error_response_exception(
                boost::beast::http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
