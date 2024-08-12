#include "put_object.h"

#include "common/utils/double_buffer.h"

using namespace boost;

namespace uh::cluster {

namespace {

coro<std::size_t> fill(http_request& req, std::vector<char>& buffer) {
    buffer.resize(buffer.capacity());

    auto read = co_await req.read_body(buffer);
    buffer.resize(read);

    co_return read;
}

std::shared_ptr<awaitable_promise<dedupe_response>>
upload(context& ctx, const reference_collection& coll,
       const std::vector<char>& buffer) {
    auto pr = std::make_shared<awaitable_promise<dedupe_response>>(coll.ioc);

    if (!buffer.empty()) {
        asio::co_spawn(coll.ioc,
                       coll.dedupe_services.get()->deduplicate(
                           ctx, {buffer.data(), buffer.size()}),
                       use_awaitable_promise_cospawn(pr));
    } else {
        pr->set(dedupe_response());
    }

    return pr;
}

} // namespace

put_object::put_object(const reference_collection& collection)
    : m_collection(collection) {}

bool put_object::can_handle(const http_request& req) {
    return req.method() == method::put &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.has_query() &&
           !req.header("x-amz-copy-source");
}

coro<http_response> put_object::handle(http_request& req) const {

    metric<entrypoint_put_object_req>::increase(1);
    http_response res;

    try {
        auto content_length = req.content_length();

        m_collection.limits.check_storage_size(content_length);
        co_await m_collection.directory.bucket_exists(req.bucket());

        if (auto expect = req.header("expect");
            expect && *expect == "100-continue") {
            LOG_INFO() << req.socket().remote_endpoint()
                       << ": sending 100 CONTINUE";
            co_await write(req.socket(),
                           http_response(http::status::continue_));
        }

        md5 hash;

        dedupe_response resp;
        if (content_length >= m_collection.config.buffer_size) {
            resp = co_await put_large_object(req, hash);
        } else {
            resp = co_await put_small_object(req, hash);
        }

        auto tag = hash.finalize();
        LOG_DEBUG() << req.socket().remote_endpoint() << ": etag: " << tag;

        object obj{.name = req.object_key(),
                   .size = resp.addr.data_size(),
                   .addr = std::move(resp.addr),
                   .etag = tag,
                   .mime = req.header("Content-Type")};
        co_await m_collection.directory.put_object(req.bucket(), obj);

        metric<entrypoint_ingested_data_counter, mebibyte, double>::increase(
            static_cast<double>(content_length) / MEBI_BYTE);

        res.set("ETag", tag);
        res.set_original_size(content_length);
        res.set_effective_size(resp.effective_size);
    } catch (const error_exception& e) {
        LOG_ERROR() << req.socket().remote_endpoint()
                    << " failed to get bucket `" << req.bucket() << "`: " << e;
        throw_from_error(e.error());
    }

    co_return res;
}

coro<dedupe_response> put_object::put_large_object(http_request& req,
                                                   md5& hash) const {
    const auto buffer_size = m_collection.config.buffer_size;
    double_buffer b(buffer_size);

    auto content_length = req.content_length();
    std::size_t transferred = 0;

    auto read = co_await fill(req, b.current());
    hash.consume(b.current());
    transferred += read;

    dedupe_response rv;

    do {
        auto promise = upload(req.context(), m_collection, b.current());
        b.flip();

        read = co_await fill(req, b.current());
        hash.consume(b.current());
        transferred += read;

        rv.append(co_await promise->get());
    } while (transferred < content_length);

    auto promise = upload(req.context(), m_collection, b.current());
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
        req.context(), {buffer.data(), buffer.size()});
}

} // namespace uh::cluster
