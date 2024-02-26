#include "delete_objects.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/command_exception.h"
#include "entrypoint/utils/xml_parser.h"

namespace uh::cluster {

delete_objects::delete_objects(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool delete_objects::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();

    return req.get_method() == method::post && !uri.get_bucket_id().empty() &&
           uri.get_object_key().empty() && uri.query_string_exists("delete");
}

auto delete_objects::validate(const http_request& req) {
    xml_parser xml_parser;
    bool parsed = xml_parser.parse(req.get_body());
    auto object_nodes = xml_parser.get_nodes("Delete.Object");

    if (!parsed || object_nodes.empty())
        throw command_exception(http::status::bad_request,
                                command_error::type::malformed_xml);

    return object_nodes;
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
        auto error = command_error::get_code_message(val.code);
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
    auto object_nodes = validate(req);

    auto bucket_id = req.get_uri().get_bucket_id();
    std::vector<std::string> success;
    std::vector<fail> failure;
    for (const auto& object : object_nodes) {
        auto key = object.get().get_optional<std::string>("Key");
        if (!key) {
            throw command_exception(http::status::bad_request,
                                    command_error::type::malformed_xml);
        }

        try {

            auto func2 = [](const std::string& key,
                            const std::string& bucket_id,
                            std::vector<std::string>& success,
                            client::acquired_messenger m) -> coro<void> {
                directory_message dir_req;
                dir_req.bucket_id = bucket_id;
                dir_req.object_key = std::make_unique<std::string>(key);

                co_await m.get().send_directory_message(
                    DIRECTORY_OBJECT_DELETE_REQ, dir_req);

                co_await m.get().recv_header();

                success.emplace_back(key);
            };

            co_await worker_utils::
                io_thread_acquire_messenger_and_post_in_io_threads(
                    m_state.workers, m_state.ioc,
                    m_state.directory_services.get(),
                    std::bind_front(func2, *key, std::cref(bucket_id),
                                    std::ref(success)));

        } catch (const error_exception& e) {
            LOG_ERROR() << "Failed to delete the bucket " << bucket_id
                        << " to the directory: " << e;
            failure.emplace_back(e.error().code(), *key);
        }
    }

    co_return get_response(success, failure);
}

} // namespace uh::cluster
