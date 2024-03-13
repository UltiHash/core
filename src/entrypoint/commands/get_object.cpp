#include "get_object.h"
#include "common/utils/worker_pool.h"
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

        auto func = [this](const http_request& req) {

            auto m = m_collection.directory_services.get()->acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = req.get_uri().get_bucket_id();
            dir_req.object_key =
                std::make_unique<std::string>(req.get_uri().get_object_key());

            m_collection.workers.get_coro_future(m.get().send_directory_message(DIRECTORY_OBJECT_GET_REQ, dir_req)).get();

            bool has_next = true;
            size_t total_size = 0;
            std::optional <std::pair <std::future <size_t>, std::string>> send_to_client;
            while (has_next) {
                auto h = m_collection.workers.get_coro_future(m.get().recv_header()).get();
                auto [b_next, buf] = m_collection.workers.get_coro_future(m.get().recv_directory_get_object_chunk(h)).get();
                total_size += buf.size();

                if (send_to_client)
                    send_to_client->first.get();
                send_to_client.emplace(boost::asio::async_write(req.socket(), http::make_chunk(boost::asio::buffer(buf.data(), buf.size())), boost::asio::use_future), std::move (buf));
                has_next = b_next;
            }
            send_to_client->first.get();
            return total_size;
        };


        http::response<http::empty_body> res {http::status::ok, 11};
        res.chunked(true);
        http::response_serializer<http::empty_body> sr (res);
        co_await http::async_write_header(req.socket(), sr, boost::asio::as_tuple(boost::asio::use_awaitable));

        const auto total_size = co_await m_collection.workers.post_in_workers(std::bind_front(func, std::cref(req)));

        co_await boost::asio::async_write(req.socket(), http::make_chunk_last(), boost::asio::as_tuple(boost::asio::use_awaitable));

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
