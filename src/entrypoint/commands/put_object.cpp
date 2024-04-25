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

    coro<std::size_t> fill(http_request& req) {
        current().resize(current().capacity());

        auto read = co_await req.read_body(current());
        current().resize(read);

        co_return read;
    }

    std::vector<char>& current() { return m_buffers[m_index]; }

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

        md5 hash;

        dedupe_response resp;
        if (content_length >= m_collection.config.buffer_size) {
            resp = co_await put_large_object(req, hash);
        } else {
            resp = co_await put_small_object(req, hash);
        }

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
        auto tag = hash.finalize();
        LOG_DEBUG() << "etag: " << tag;

        res.set_etag(tag);
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

coro<dedupe_response> put_object::put_large_object(http_request& req,
                                                   md5& hash) const {
    const auto buffer_size = m_collection.config.buffer_size;
    double_buffer b(m_collection, buffer_size);

    auto content_length = req.content_length();
    std::size_t transferred = 0;

    auto read = co_await b.fill(req);
    hash.consume(b.current());
    transferred += read;

    dedupe_response rv;

    do {
        auto promise = b.upload();
        b.flip();

        auto read = co_await b.fill(req);
        hash.consume(b.current());
        transferred += read;

        rv.append(co_await promise->get());
    } while (transferred < content_length);

    auto promise = b.upload();
    rv.append(co_await promise->get());

    co_return rv;
}

coro<dedupe_response> put_object::put_small_object(http_request& req,
                                                   md5& hash) const {
    auto content_length = req.content_length();

    std::vector<char> buffer(content_length);
    auto read = co_await req.read_body(buffer);
    buffer.resize(read);
    hash.consume(buffer);

    if (buffer.empty()) {
        co_return dedupe_response();
    }

    co_return co_await integration::integrate_data(buffer, m_collection);
}

} // namespace uh::cluster
