
#ifndef UH_CLUSTER_LIST_OBJECTSV2_H
#define UH_CLUSTER_LIST_OBJECTSV2_H

#include "common/utils/worker_utils.h"
#include "entrypoint/common.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"
#include "entrypoint/rest/utils/string/string_utils.h"
#include "http_request.h"
#include "http_response.h"

namespace uh::cluster {

class list_objects_v2 {

    explicit list_objects_v2(entrypoint_state& state) : m_state(state) {}

    static bool can_handle(const http_request& req) {
        if (req.get_method() == method::get) {
            if (const auto& uri = req.get_URI();
                uri.query_string_exists("list-type") &&
                uri.get_query_string_value("list-type") == "2") {
                return true;
            }
        }
        return false;
    }

    coro<http_response> handle(const http_request& req) {
        try {
            std::vector<std::string> content;

            auto func = [](const http_request& req,
                           std::vector<std::string>& content,
                           client::acquired_messenger m) -> coro<void> {
                const auto& uri = req.get_URI();

                directory_message dir_req;
                dir_req.bucket_id = uri.get_bucket_id();
                if (uri.query_string_exists("prefix")) {
                    if (const auto& prefix =
                            uri.get_query_string_value("prefix");
                        !prefix.empty()) {
                        dir_req.object_key_prefix =
                            std::make_unique<std::string>(prefix);
                    }
                }
                if (uri.query_string_exists("start-after")) {
                    if (const auto& start_after =
                            uri.get_query_string_value("start-after");
                        !start_after.empty()) {
                        dir_req.object_key_lower_bound =
                            std::make_unique<std::string>(start_after);
                    }
                }

                co_await m.get().send_directory_message(DIR_LIST_OBJ_REQ,
                                                        dir_req);
                const auto h_dir = co_await m.get().recv_header();

                unique_buffer<char> buffer(h_dir.size);
                directory_lst_entities_message list_objects_res;

                list_objects_res =
                    co_await m.get().recv_directory_list_entities_message(
                        h_dir);

                for (const auto& entity : list_objects_res.entities) {
                    content.emplace_back(entity);
                }
            };

            co_await worker_utils::
                io_thread_acquire_messenger_and_post_in_io_threads(
                    m_state.workers, m_state.ioc,
                    m_state.directory_services.get(),
                    std::bind_front(func, std::cref(req), std::ref(content)));
            co_return get_response(content, req);

        } catch (const error_exception& e) {
            LOG_ERROR() << e.what();
            switch (*e.error()) {
            case error::bucket_not_found:
                throw rest::http::model::custom_error_response_exception(
                    boost::beast::http::status::not_found,
                    rest::http::model::error::bucket_not_found);
            default:
                throw rest::http::model::custom_error_response_exception(
                    boost::beast::http::status::internal_server_error);
            }
        }
    }

    http_response get_response(const std::vector<std::string>& content,
                               const http_request& req) {

        http_response res;
        const auto& uri = req.get_URI();

        int max_keys = 0;
        if (uri.query_string_exists("max-keys")) {
            max_keys = std::stoi(uri.get_query_string_value("max-keys"));
        }

        const auto& fetch_owner = uri.get_query_string_value("fetch-owner");
        bool fetch_owner_set = false;
        if (!fetch_owner.empty()) {
            if (rest::utils::string_utils::is_bool(fetch_owner))
                fetch_owner_set = true;
        }

        std::string start_after;
        if (uri.query_string_exists("start-after")) {
            start_after = uri.get_query_string_value("start-after");
        }

        std::string prefix;
        if (uri.query_string_exists("prefix")) {
            prefix = uri.get_query_string_value("prefix");
        }

        const auto& delimeter = uri.get_query_string_value("delimiter");

        const auto& encoding_type = uri.get_query_string_value("encoding-type");

        const auto& m_continuation_token =
            uri.get_query_string_value("continuation-token");

        std::set<std::string> common_prefixes;
        std::string content_xml_string;

        size_t counter = 0;
        if (!content.empty() && max_keys != 0) {

            for (const auto& c : content) {

                if (!delimeter.empty()) {
                    size_t delimiter_index;
                    if (!prefix.empty()) {
                        delimiter_index = c.find(delimeter, prefix.size());
                    } else {
                        delimiter_index = c.find(delimeter);
                    }

                    if (delimiter_index != std::string::npos) {
                        auto delimeter_prefix =
                            c.substr(0, delimiter_index + 1);
                        common_prefixes.emplace(
                            (encoding_type.empty()
                                 ? delimeter_prefix
                                 : rest::utils::string_utils::URL_encode(
                                       delimeter_prefix)));
                    }

                } else {
                    content_xml_string +=
                        "<Contents>\n"
                        "<Key>" +
                        (encoding_type.empty()
                             ? c
                             : rest::utils::string_utils::URL_encode(c)) +
                        "</Key>\n" +
                        (fetch_owner_set ? "<Owner>no-owner-support</Owner>"
                                         : "") +
                        "</Contents>\n";
                }
                counter++;

                if (counter + common_prefixes.size() == max_keys)
                    break;
            }
        }

        // common prefixes string
        std::string common_prefixes_xml_string;
        for (const auto& common_prefix : common_prefixes) {
            common_prefixes_xml_string += "<CommonPrefixes>\n<Prefix>" +
                                          common_prefix +
                                          "</Prefix>\n</CommonPrefixes>\n";
        }

        std::string delimiter_xml_string;
        if (!delimeter.empty()) {
            delimiter_xml_string =
                "<Delimiter>" +
                (encoding_type.empty()
                     ? delimeter
                     : rest::utils::string_utils::URL_encode(delimeter)) +
                "</Delimiter>\n";
        }

        std::string key_count_xml;
        if (!content.empty()) {
            content_xml_string +=
                "<KeyCount>" +
                std::to_string(counter + common_prefixes.size()) +
                "</KeyCount>\n";
        }

        std::string name_xml_string;

        std::string truncated = "false";
        if (content.size() > max_keys && max_keys != 0)
            truncated = "true";

        std::string max_keys_xml_string =
            "<MaxKeys>" + std::to_string(max_keys) + "</MaxKeys>\n";

        std::string encoding_type_xml_string;
        if (!encoding_type.empty()) {
            encoding_type_xml_string =
                "<EncodingType>" + encoding_type + "</EncodingType>\n";
        }

        std::string continuation_token_xml_string;
        if (!m_continuation_token.empty()) {
            continuation_token_xml_string = "<ContinuationToken>" +
                                            m_continuation_token +
                                            "</ContinuationToken>\n";
        }

        std::string start_after_xml_string;
        if (!start_after.empty()) {
            encoding_type_xml_string =
                "<StartAfter>" + start_after + "</StartAfter>\n";
        }

        std::string prefix_xml_string;
        if (!prefix.empty()) {
            prefix_xml_string =
                "<Prefix>" +
                (encoding_type.empty()
                     ? prefix
                     : rest::utils::string_utils::URL_encode(prefix)) +
                "</Prefix>\n";
        }

        res.set_body(
            std::string("<ListBucketResult>\n"
                        "<IsTruncated>" +
                        truncated + "</IsTruncated>\n" + content_xml_string +
                        name_xml_string + prefix_xml_string +
                        delimiter_xml_string + max_keys_xml_string +
                        common_prefixes_xml_string + encoding_type_xml_string +
                        key_count_xml + continuation_token_xml_string +
                        start_after_xml_string + "</ListBucketResult>"));

        return res;
    }

    entrypoint_state& m_state;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LIST_OBJECTSV2_H
