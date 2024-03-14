
#ifndef UH_CLUSTER_LIST_OBJECTSV2_H
#define UH_CLUSTER_LIST_OBJECTSV2_H

#include "common/utils/strings.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class list_objects_v2 {

public:
    explicit list_objects_v2(const reference_collection& collection)
        : m_collection(collection) {}

    static bool can_handle(const http_request& req) {
        const auto& uri = req.get_uri();

        return req.get_method() == method::get &&
               !uri.get_bucket_id().empty() && uri.get_object_key().empty() &&
               uri.query_string_exists("list-type") &&
               uri.get_query_string_value("list-type") == "2";
    }

    coro<void> handle(http_request& req) {
        metric<entrypoint_list_objects_v2_req>::increase(1);
        try {
            const auto& req_uri = req.get_uri();
            directory_message dir_req;
            dir_req.bucket_id = req_uri.get_bucket_id();

            if (req_uri.query_string_exists("prefix")) {
                if (const auto& prefix =
                        req_uri.get_query_string_value("prefix");
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

            directory_list_objects_message list_objs_res;

            auto func = [](const directory_message& dir_req,
                           directory_list_objects_message& list_objs_res,
                           client::acquired_messenger m) -> coro<void> {
                co_await m.get().send_directory_message(
                    DIRECTORY_OBJECT_LIST_REQ, dir_req);
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

    static http_response get_response(const std::vector<object>& objects,
                                      const http_request& req) {

        const auto& req_uri = req.get_uri();

        const auto get_if_exists =
            [&req_uri](auto&& key) -> std::optional<std::string> {
            if (req_uri.query_string_exists(key)) {
                if (auto& val = req_uri.get_query_string_value(key);
                    !val.empty())
                    return std::make_optional<std::string>(val);
            }
            return std::nullopt;
        };

        const auto start_after = get_if_exists("start-after");
        const auto prefix = get_if_exists("prefix");
        const auto delimiter = get_if_exists("delimiter");
        const auto encoding_type = get_if_exists("encoding-type");
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

        std::set<std::string> common_prefixes;
        std::string content_xml_string;

        size_t counter = 0;
        if (!objects.empty() && max_keys != 0) {

            for (std::size_t tally = 0; const auto& obj : objects) {
                size_t delimiter_index = std::string::npos;

                if (delimiter) {
                    if (prefix) {
                        delimiter_index =
                            obj.name.find(*delimiter, prefix->size());
                    } else {
                        delimiter_index = obj.name.find(*delimiter);
                    }
                }
                if (delimiter_index != std::string::npos) {
                    auto delimiter_prefix =
                        obj.name.substr(0, delimiter_index + 1);
                    common_prefixes.emplace((encoding_type
                                                 ? url_encode(delimiter_prefix)
                                                 : delimiter_prefix));
                } else {
                    content_xml_string +=
                        "<Contents>\n"
                        "<LastModified>" +
                        objects[tally].last_modified +
                        "</LastModified>\n"
                        "<Key>" +
                        (encoding_type ? url_encode(obj.name) : obj.name) +
                        "</Key>\n" +
                        (fetch_owner_set ? "<Owner>no-owner-support</Owner>"
                                         : "") +
                        "<Size>" + std::to_string(objects[tally].size) +
                        "</Size>\n"
                        "</Contents>\n";
                    counter++;
                }

                if (counter + common_prefixes.size() == max_keys)
                    break;

                tally++;
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
        if (delimiter) {
            delimiter_xml_string =
                "<Delimiter>" +
                (encoding_type ? url_encode(*delimiter) : *delimiter) +
                "</Delimiter>\n";
        }

        std::string key_count_xml;
        content_xml_string += "<KeyCount>" +
                              std::to_string(counter + common_prefixes.size()) +
                              "</KeyCount>\n";

        std::string name_xml_string;

        std::string truncated = "false";
        if (objects.size() > max_keys && max_keys != 0)
            truncated = "true";

        std::string max_keys_xml_string =
            "<MaxKeys>" + std::to_string(max_keys) + "</MaxKeys>\n";

        std::string encoding_type_xml_string;
        if (encoding_type) {
            encoding_type_xml_string =
                "<EncodingType>" + *encoding_type + "</EncodingType>\n";
        }

        std::string continuation_token_xml_string;
        if (continuation_token) {
            continuation_token_xml_string = "<ContinuationToken>" +
                                            *continuation_token +
                                            "</ContinuationToken>\n";
        }

        std::string start_after_xml_string;
        if (start_after) {
            encoding_type_xml_string =
                "<StartAfter>" + *start_after + "</StartAfter>\n";
        }

        std::string prefix_xml_string;
        if (prefix) {
            prefix_xml_string =
                "<Prefix>" + (encoding_type ? url_encode(*prefix) : *prefix) +
                "</Prefix>\n";
        }

        http_response res;
        res.set_body("<ListBucketResult>\n"
                     "<IsTruncated>" +
                     truncated + "</IsTruncated>\n" + content_xml_string +
                     name_xml_string + prefix_xml_string +
                     delimiter_xml_string + max_keys_xml_string +
                     common_prefixes_xml_string + encoding_type_xml_string +
                     key_count_xml + continuation_token_xml_string +
                     start_after_xml_string + "</ListBucketResult>");

        return res;
    }

private:
    const reference_collection& m_collection;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LIST_OBJECTSV2_H
