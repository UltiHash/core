#include "get_object.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

get_object::get_object(const reference_collection& collection)
    : m_collection(collection) {}

bool get_object::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::get && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() &&
           !uri.query_string_exists("attributes");
}

coro<http_response> get_object::handle(const http_request& req) const {
    metric<entrypoint_get_object_req>::increase(1);
    try {
        std::chrono::time_point<std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now();
        std::string buffer;

        auto func = [](std::string& buffer, const http_request& req,
                       client::acquired_messenger m) -> coro<void> {
            directory_message dir_req;
            dir_req.bucket_id = req.get_uri().get_bucket_id();
            dir_req.object_key =
                std::make_unique<std::string>(req.get_uri().get_object_key());

            co_await m.get().send_directory_message(DIRECTORY_OBJECT_GET_REQ,
                                                    dir_req);
            const auto h_dir = co_await m.get().recv_header();

            buffer.resize(h_dir.size);
            m.get().register_read_buffer(buffer);
            co_await m.get().recv_buffers(h_dir);
        };

        co_await worker_utils::
            io_thread_acquire_messenger_and_post_in_io_threads(
                m_collection.workers, m_collection.ioc,
                m_collection.directory_services.get(),
                std::bind_front(func, std::ref(buffer), std::cref(req)));

        const auto stop = std::chrono::steady_clock::now();
        const std::chrono::duration<double> duration = stop - start;
        const auto size = static_cast<double>(buffer.size()) / MEBI_BYTE;
        const auto bandwidth = size / duration.count();

        metric<entrypoint_egressed_data_counter, byte>::increase(buffer.size());

        LOG_DEBUG() << "retrieval duration " << duration.count() << " s";
        LOG_DEBUG() << "retrieval bandwidth " << bandwidth << " MB/s";

        http_response res;
        res.set_body(std::move(buffer));
        res.set_bandwidth(bandwidth);

        co_return res;

    } catch (const error_exception& e) {
        LOG_ERROR() << e.what();
        switch (*e.error()) {
        case error::object_not_found:
            throw command_exception(http::status::not_found,
                                    command_error::object_not_found);
        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
