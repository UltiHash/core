#include "put_object.h"

namespace uh::cluster {

put_object::put_object(const reference_collection& collection)
    : m_collection(collection) {}

bool put_object::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::put && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

coro<http_response> put_object::handle(http_request& req) const {
    metric<entrypoint_put_object_req>::increase(1);
    try {
        co_await req.read_body();
        const auto start = std::chrono::steady_clock::now();

        auto body_size = req.get_body_size();
        const auto size_mb = static_cast<double>(body_size) /
                             static_cast<double>(1024ul * 1024ul);

        dedupe_response resp{.effective_size = 0};
        if (body_size > 0) [[likely]] {
            std::list<std::string_view> data{req.get_body()};
            resp = co_await integration::integrate_data(data, m_collection);
        }

        const directory_message dir_req{
            .bucket_id = req.get_uri().get_bucket_id(),
            .object_key =
                std::make_unique<std::string>(req.get_uri().get_object_key()),
            .addr = std::make_unique<address>(std::move(resp.addr)),
        };

        auto directories = m_collection.directory_services.get_clients();
        if (directories.empty()) {
            throw std::runtime_error("no directory services available");
        }

        auto func = [](const directory_message& dir_req,
                       client::acquired_messenger m, long id) -> coro<void> {
            co_await m.get().send_directory_message(DIRECTORY_OBJECT_PUT_REQ,
                                                    dir_req);
            co_await m.get().recv_header();
        };

        co_await m_collection.workers.broadcast_from_io_thread_in_io_threads(
            directories, std::bind_front(func, std::cref(dir_req)));

        double effective_size = 0;
        double space_saving = 0;
        if (body_size) {
            effective_size =
                static_cast<double>(resp.effective_size) / MEBI_BYTE;
            space_saving = 1.0 - static_cast<double>(resp.effective_size) /
                                     static_cast<double>(body_size);
        }

        const auto stop = std::chrono::steady_clock::now();
        const std::chrono::duration<double> duration = stop - start;
        const auto bandwidth = size_mb / duration.count();

        metric<entrypoint_ingested_data_counter, mebibyte, double>::increase(
            size_mb);

        LOG_INFO() << "original size " << size_mb << " MB\n"
                   << "effective size " << effective_size << " MB\n"
                   << "space saving " << space_saving << '\n'
                   << "integration duration " << duration.count() << " s\n"
                   << "integration bandwidth " << bandwidth << " MB/s";

        http_response res;
        res.set_original_size(req.get_body_size());
        res.set_effective_size(resp.effective_size);
        res.set_space_savings(space_saving);
        res.set_bandwidth(bandwidth);

        co_return res;

    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to get bucket `" << req.get_uri().get_bucket_id()
                    << "`: " << e;
        switch (*e.error()) {
        case error::bucket_not_found:
            throw command_exception(boost::beast::http::status::not_found,
                                    command_error::bucket_not_found);
        case error::storage_limit_exceeded:
            throw command_exception(http::status::insufficient_storage,
                                    command_error::insufficient_storage);

        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
