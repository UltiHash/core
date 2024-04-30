#include "get_object.h"
#include "common/coroutines/awaitable_promise.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

get_object::get_object(const reference_collection& collection)
    : m_collection(collection) {}

bool get_object::can_handle(const http_request& req) {
    return req.method() == method::get && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("attributes");
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

        auto dir = m_collection.directory_services.get();

        auto read_handler =
            co_await dir->get_object(req.bucket(), req.object_key());

        size_t total_size = 0;

        std::shared_ptr<awaitable_promise<std::size_t>> promise;
        std::string buffer;

        while (read_handler->has_next()) {

            auto data = co_await read_handler->next();
            total_size += data.size();

            if (promise) {
                co_await promise->get();
            }

            buffer = std::move(data);
            promise = std::make_shared<awaitable_promise<std::size_t>>(
                m_collection.ioc);

            boost::asio::async_write(req.socket(),
                                     http::make_chunk(boost::asio::buffer(
                                         buffer.data(), buffer.size())),
                                     use_awaitable_promise(promise));
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
        throw_from_error(e.error());
    }
}

} // namespace uh::cluster
