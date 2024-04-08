#include "put_object.h"

#include "common/utils/awaitable_promise.h"

using namespace boost;

namespace uh::cluster {

namespace {

class double_buffer {
public:
    double_buffer(const reference_collection& collection, std::size_t size)
        : m_collection(collection),
          m_buffers(),
          m_index(0) {
        m_buffers[0].reserve(size);
        m_buffers[1].reserve(size);
    }

    void flip() { m_index = m_index == 0 ? 1 : 0; }

    std::vector<char>& current() { return m_buffers[m_index]; }

    coro<std::size_t> fill(asio::ip::tcp::socket& sock, std::size_t size,
                           std::size_t offset = 0) {
        auto& b = current();
        b.resize(offset + size);

        return asio::async_read(sock, asio::buffer(&b[offset], size),
                                asio::use_awaitable);
    }

    std::shared_ptr<awaitable_promise<dedupe_response>> upload() {
        auto pr = std::make_shared<awaitable_promise<dedupe_response>>(
            m_collection.ioc);

        auto& b = current();

        if (!b.empty()) {
            asio::co_spawn(m_collection.ioc,
                           integration::integrate_data(b, m_collection),
                           use_awaitable_promise_cospawn(pr));
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
        auto content_length = req.content_length();

        dedupe_response resp;
        if (content_length >= m_collection.config.buffer_size) {
            resp = co_await put_large_object(req);
        } else {
            resp = co_await put_small_object(req);
        }

        md5 hash;
        hash.consume({reinterpret_cast<const char*>(&resp.addr.pointers[0]),
                      resp.addr.pointers.size() * sizeof(uint64_t)});
        hash.consume({reinterpret_cast<const char*>(&resp.addr.sizes[0]),
                      resp.addr.sizes.size() * sizeof(uint32_t)});

        const directory_message dir_req{
            .bucket_id = req.get_uri().get_bucket_id(),
            .object_key =
                std::make_unique<std::string>(req.get_uri().get_object_key()),
            .addr = std::make_unique<address>(std::move(resp.addr)),
        };

        auto func = [&](acquired_messenger<coro_client> m,
                        long id) -> coro<void> {
            co_await m->send_directory_message(DIRECTORY_OBJECT_PUT_REQ,
                                               dir_req);
            co_await m->recv_header();
        };

        co_await m_collection.directory_services.broadcast(func);

        metric<entrypoint_ingested_data_counter, mebibyte, double>::increase(
            static_cast<double>(content_length) / MEBI_BYTE);

        http_response res;

        res.set_etag(hash.finalize());
        res.set_original_size(content_length);
        res.set_effective_size(resp.effective_size);

        co_await req.respond(res.get_prepared_response());

    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to get bucket `" << req.get_uri().get_bucket_id()
                    << "`: " << e;
        switch (*e.error()) {
        case error::bucket_not_found:
            throw command_exception(beast::http::status::not_found,
                                    command_error::bucket_not_found);
        case error::storage_limit_exceeded:
            throw command_exception(http::status::insufficient_storage,
                                    command_error::insufficient_storage);

        default:
            throw command_exception(http::status::internal_server_error);
        }
    }
}

coro<dedupe_response> put_object::put_large_object(http_request& req) const {
    const auto buffer_size = m_collection.config.buffer_size;
    double_buffer b(m_collection, buffer_size);

    auto content_length = req.content_length();

    std::size_t transferred = req.payload().size();
    b.current().resize(transferred);
    asio::buffer_copy(asio::buffer(b.current()), req.payload().data());

    auto size = std::min(content_length, buffer_size) - transferred;
    transferred += co_await b.fill(req.socket(), size, transferred);

    dedupe_response rv;

    do {
        auto promise = b.upload();

        b.flip();

        size = std::min(content_length - transferred, buffer_size);
        transferred += co_await b.fill(req.socket(), size);

        rv.append(co_await promise->get());
    } while (transferred < content_length);

    auto promise = b.upload();
    rv.append(co_await promise->get());

    co_return rv;
}

coro<dedupe_response> put_object::put_small_object(http_request& req) const {
    auto content_length = req.content_length();

    if (content_length == 0) {
        co_return dedupe_response();
    }

    std::vector<char> buffer(content_length);

    auto& payload = req.payload();
    asio::buffer_copy(asio::buffer(buffer), payload.data(), payload.size());

    (void)co_await asio::async_read(
        req.socket(),
        asio::buffer(&buffer[payload.size()], content_length - payload.size()),
        asio::use_awaitable);

    co_return co_await integration::integrate_data(buffer, m_collection);
}

} // namespace uh::cluster
