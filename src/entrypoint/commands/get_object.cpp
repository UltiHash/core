#include "get_object.h"
#include "common/utils/awaitable_promise.h"
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

coro<void> get_object::handle(http_request& req) const {
    metric<entrypoint_get_object_req>::increase(1);
    try {
        std::chrono::time_point<std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now();

        http::response<http::empty_body> res{http::status::ok, 11};
        res.chunked(true);
        http::response_serializer<http::empty_body> sr(res);
        co_await http::async_write_header(
            req.socket(), sr,
            boost::asio::as_tuple(boost::asio::use_awaitable));

        auto m =
            co_await m_collection.directory_services.get()->acquire_messenger();

        directory_message dir_req;
        dir_req.bucket_id = req.get_uri().get_bucket_id();
        dir_req.object_key =
            std::make_unique<std::string>(req.get_uri().get_object_key());

        co_await m->send_directory_message(DIRECTORY_OBJECT_GET_REQ, dir_req);

        bool has_next = true;
        size_t total_size = 0;

        std::shared_ptr<awaitable_promise<std::size_t>> promise;
        std::string buffer;

        while (has_next) {
            auto h = co_await m->recv_header();
            auto [b_next, buf] = co_await m->recv_directory_get_object_chunk(h);
            total_size += buf.size();

            if (promise) {
                co_await promise->get();
            }

            buffer = std::move(buf);
            promise = std::make_shared<awaitable_promise<std::size_t>>(
                m_collection.ioc);

            boost::asio::async_write(req.socket(),
                                     http::make_chunk(boost::asio::buffer(
                                         buffer.data(), buffer.size())),
                                     use_awaitable_promise(promise));

            has_next = b_next;
        }

        if (promise) {
            co_await promise->get();
            promise.reset();
        }

        co_await boost::asio::async_write(
            req.socket(), http::make_chunk_last(),
            boost::asio::as_tuple(boost::asio::use_awaitable));

        const auto stop = std::chrono::steady_clock::now();
        const std::chrono::duration<double> duration = stop - start;
        const auto size = static_cast<double>(total_size) / MEBI_BYTE;
        const auto bandwidth = size / duration.count();

        metric<entrypoint_egressed_data_counter, byte>::increase(total_size);

        LOG_INFO() << "retrieval duration " << duration.count() << " s";
        LOG_INFO() << "retrieval bandwidth " << bandwidth << " MB/s";

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
