#include "delete_objects.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

delete_objects::delete_objects(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool delete_objects::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::post && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.query_string_exists("delete");
}

pugi::xpath_node_set delete_objects::validate(const http_request& req) {
    pugi::xpath_node_set object_nodes_set;

    rest::utils::parser::xml_parser parsed_xml;
    try {
        if (!parsed_xml.parse(req.get_body()))
            throw std::runtime_error("");

        object_nodes_set = parsed_xml.get_nodes_from_path("/Delete/Object");
        if (object_nodes_set.empty())
            throw std::runtime_error("");
    } catch (const std::exception& e) {
        throw rest::http::model::custom_error_response_exception(
            boost::beast::http::status::bad_request,
            rest::http::model::error::type::malformed_xml);
    }

    return object_nodes_set;
}

namespace {
struct fail {
    uint32_t code;
    std::string key;
};

http_response get_response(const std::vector<std::string>& success,
                           const std::vector<fail>& failure) noexcept {
    http_response res;

    std::string xml_string;
    for (const auto& val : success) {
        xml_string += "<Deleted>\n"
                      "<Key>" +
                      val +
                      "</Key>\n"
                      "</Deleted>\n";
    }
    for (const auto& val : failure) {
        auto error = rest::http::model::error::get_code_message(val.code);
        xml_string += "<Error>\n"
                      "<Key>" +
                      error.first +
                      "</Key>\n"
                      "<Code>" +
                      error.second +
                      "</Code>\n"
                      "</Error>\n";
    }

    res.set_body(std::string("<DeleteResult>\n" + std::move(xml_string) +
                             "</DeleteResult>"));

    return res;
}
} // namespace

coro<http_response> delete_objects::handle(http_request& req) const {
    co_await req.read_body();
    auto object_nodes_set = validate(req);

    auto bucket_id = req.get_uri().get_bucket_id();
    std::vector<std::string> success;
    std::vector<fail> failure;
    for (const auto& objectNode : object_nodes_set) {
        auto key = objectNode.node().child("Key").child_value();

        try {

            auto func2 = [](const char* key, const std::string& bucket_id,
                            std::vector<std::string>& success,
                            client::acquired_messenger m) -> coro<void> {
                directory_message dir_req;
                dir_req.bucket_id = bucket_id;
                dir_req.object_key = std::make_unique<std::string>(key);

                co_await m.get().send_directory_message(DIR_DELETE_OBJ_REQ,
                                                        dir_req);

                co_await m.get().recv_header();

                success.emplace_back(key);
            };

            co_await worker_utils::
                io_thread_acquire_messenger_and_post_in_io_threads(
                    m_state.workers, m_state.ioc,
                    m_state.directory_services.get(),
                    std::bind_front(func2, key, std::cref(bucket_id),
                                    std::ref(success)));

        } catch (const error_exception& e) {
            LOG_ERROR() << "Failed to delete the bucket " << bucket_id
                        << " to the directory: " << e;
            failure.emplace_back(e.error().code(), key);
        }
    }

    co_return get_response(success, failure);
}

} // namespace uh::cluster
