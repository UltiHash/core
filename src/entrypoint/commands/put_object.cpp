#include "put_object.h"

#include "common/utils/awaitable_promise.h"

namespace uh::cluster {

namespace {

class buffers {
public:
    buffers(const reference_collection& collection, std::size_t size)
        : m_collection(collection),
          m_buffers(),
          m_index(0) {
        m_buffers[0].reserve(size);
        m_buffers[1].reserve(size);
    }

    void flip() { m_index = m_index == 0 ? 1 : 0; }

    std::vector<char>& current() { return m_buffers[m_index]; }

    coro<std::size_t> fill(boost::asio::ip::tcp::socket& sock, std::size_t size,
                           std::size_t offset = 0) {
        auto& b = current();
        b.resize(offset + size);

        return boost::asio::async_read(sock,
                                       boost::asio::buffer(&b[offset], size),
                                       boost::asio::use_awaitable);
    }

    std::shared_ptr<awaitable_promise<dedupe_response>> upload() {
        auto pr = std::make_shared<awaitable_promise<dedupe_response>>(
            m_collection.ioc);

        auto& b = current();

        if (!b.empty()) {
            std::list<std::string_view> pieces{
                std::string_view(b.begin(), b.end())};
            boost::asio::co_spawn(
                m_collection.ioc,
                integration::integrate_data(pieces, m_collection),
                use_awaitable_promise(pr));
        } else {
            pr->set(dedupe_response());
        }

        return pr;
    }

private:
    const reference_collection& m_collection;
    std::array<std::vector<char>, 2> m_buffers;
    unsigned m_index;
};

} // namespace

put_object::put_object(const reference_collection& collection)
    : m_collection(collection) {}

bool put_object::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::put && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() && uri.get_query_parameters().empty();
}

coro<void> put_object::handle(http_request& req) const {
    metric<entrypoint_put_object_req>::increase(1);
    try {
        buffers b(m_collection, m_buffer_size);

        auto content_length = req.content_length();

        std::size_t transferred = req.payload_begin().size();
        b.current().resize(transferred);
        auto asio_b = boost::asio::buffer(b.current());
        boost::asio::buffer_copy(asio_b, req.payload_begin().data());

        auto size = std::min(content_length, m_buffer_size) - transferred;
        transferred += co_await b.fill(req.socket(), size, transferred);

        address addr;
        std::size_t effective_size = 0ull;

        do {
            auto promise = b.upload();

            b.flip();

            auto size = std::min(content_length - transferred, m_buffer_size);
            transferred += co_await b.fill(req.socket(), size);

            auto resp = co_await promise->get();
            addr.append_address(resp.addr);
            effective_size += resp.effective_size;
        } while (transferred < content_length);

        if (!b.current().empty()) {
            auto promise = b.upload();
            auto resp = co_await promise->get();
            addr.append_address(resp.addr);
            effective_size += resp.effective_size;
        }

        const directory_message dir_req{
            .bucket_id = req.get_uri().get_bucket_id(),
            .object_key =
                std::make_unique<std::string>(req.get_uri().get_object_key()),
            .addr = std::make_unique<address>(addr),
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

        metric<entrypoint_ingested_data_counter, mebibyte, double>::increase(
            static_cast<double>(content_length) / MEBI_BYTE);

        http_response res;

        md5 hash;
        hash.consume({reinterpret_cast<const char*>(&addr.pointers[0]),
                      addr.pointers.size() * sizeof(uint64_t)});
        hash.consume({reinterpret_cast<const char*>(&addr.sizes[0]),
                      addr.sizes.size() * sizeof(uint32_t)});

        res.set_etag(hash.finalize());
        res.set_original_size(content_length);
        res.set_effective_size(effective_size);

        co_await req.respond(res.get_prepared_response());

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
