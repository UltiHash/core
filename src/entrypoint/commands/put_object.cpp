#include "put_object.h"

#include "common/coroutines/awaitable_promise.h"
#include "common/coroutines/coro_utils.h"

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

    auto& current() { return m_buffers[m_index]; }

    coro<std::size_t> fill(http_request& req) {
        current().resize(current().capacity());

        auto read = co_await req.read_body(current().span());
        current().resize(read);

        co_return read;
    }


    std::shared_ptr<awaitable_promise<dedupe_response>> upload() {
        auto pr = std::make_shared<awaitable_promise<dedupe_response>>(
            m_collection.ioc);

        auto& b = current();

        if (!b.empty()) {
            asio::co_spawn(m_collection.ioc,
                           m_collection.dedupe_services.get()->deduplicate(
                               {b.data(), b.size()}),
                           use_awaitable_promise_cospawn(pr));
        } else {
            pr->set(dedupe_response());
        }

        return pr;
    }

private:
    const reference_collection& m_collection;
    std::array<unique_buffer<char>, 2> m_buffers;
    unsigned m_index;
};

} // namespace

put_object::put_object(const reference_collection& collection)
    : m_collection(collection) {}

bool put_object::can_handle(const http_request& req) {
    return req.method() == method::put && !req.bucket().empty() &&
           !req.object_key().empty() && !req.has_query();
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

        auto func = [&req, &resp](std::shared_ptr<directory_interface> dir,
                                  size_t id) -> coro<void> {
            co_await dir->put_object(req.bucket(), req.object_key(), resp.addr);
        };

        co_await broadcast<directory_interface>(
            m_collection.ioc, func,
            m_collection.directory_services.get_services());

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
        LOG_ERROR() << "Failed to get bucket `" << req.bucket() << "`: " << e;
        throw_from_error(e.error());
    }
}

coro<dedupe_response> put_object::put_large_object(http_request& req,
                                                   md5& hash) const {
    const auto buffer_size = m_collection.config.buffer_size;
    double_buffer b(m_collection, buffer_size);

    auto content_length = req.content_length();
    std::size_t transferred = 0;

    auto read = co_await b.fill(req);
    hash.consume(b.current().span());
    transferred += read;

    dedupe_response rv;

    do {
        auto promise = b.upload();
        b.flip();

        read = co_await b.fill(req);
        hash.consume(b.current().span());
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

    unique_buffer<char> buffer(content_length);
    auto read = co_await req.read_body(buffer.span());
    buffer.resize(read);
    hash.consume(buffer.span());

    if (buffer.empty()) {
        co_return dedupe_response();
    }

    co_return co_await m_collection.dedupe_services.get()->deduplicate(
        {buffer.data(), buffer.size()});
}

} // namespace uh::cluster
