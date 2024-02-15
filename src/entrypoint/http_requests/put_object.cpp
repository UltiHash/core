#include "put_object.h"

namespace uh::cluster {

put_object::put_object(entrypoint_state& entry_state) : m_state(entry_state) {}

bool put_object::can_handle(const http_request& req) {
    if (req.get_method() == method::put) {

        const auto& uri = req.get_URI();
        if (!uri.get_bucket_id().empty() && !uri.get_object_key().empty() &&
            uri.get_query_parameters().empty()) {
            return true;
        }
    }

    return false;
}

coro<http_response> put_object::handle(http_request& req) const {

    try {
        co_await req.read_body();
        std::chrono::time_point<std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now();

        auto body_size = req.get_body_size();
        const auto size_mb = static_cast<double>(body_size) /
                             static_cast<double>(1024ul * 1024ul);

        dedupe_response resp{.effective_size = 0};
        if (body_size > 0) [[likely]] {
            std::list<std::string_view> data{req.get_body()};
            resp = co_await integration::integrate_data(data, m_state);
        }

        const directory_message dir_req{
            .bucket_id = req.get_URI().get_bucket_id(),
            .object_key =
                std::make_unique<std::string>(req.get_URI().get_object_key()),
            .addr = std::make_unique<address>(std::move(resp.addr)),
        };

        auto func = [](const directory_message& dir_req,
                       client::acquired_messenger m, long id) -> coro<void> {
            co_await m.get().send_directory_message(DIRECTORY_OBJECT_PUT_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };
        co_await worker_utils::broadcast_from_io_thread_in_io_threads(
            m_state.directory_services.get_clients(), m_state.ioc,
            m_state.workers, std::bind_front(func, std::cref(dir_req)));

        auto effective_size = static_cast<double>(resp.effective_size) /
                              static_cast<double>(1024ul * 1024ul);
        auto space_saving = 1.0 - static_cast<double>(resp.effective_size) /
                                      static_cast<double>(body_size);
        const auto stop = std::chrono::steady_clock::now();
        const std::chrono::duration<double> duration = stop - start;
        const auto bandwidth = size_mb / duration.count();

        LOG_INFO() << "original size " << size_mb << " MB\n"
                   << "effective size " << effective_size << " MB\n"
                   << "space saving " << space_saving << '\n'
                   << "integration duration " << duration.count() << " s\n"
                   << "integration bandwidth " << bandwidth << " MB/s";

        http_response res;
        res.set_effective_size(resp.effective_size);
        res.set_space_savings(space_saving);
        res.set_bandwidth(bandwidth);

        co_return res;

    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to get bucket `" << req.get_URI().get_bucket_id()
                    << "`: " << e;
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

} // namespace uh::cluster
