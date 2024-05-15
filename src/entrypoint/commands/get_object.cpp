#include "get_object.h"
#include "common/coroutines/awaitable_promise.h"
#include "common/utils/double_buffer.h"
#include "common/utils/time_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

namespace {

struct local_read_handle {
    global_data_view& m_storage;
    const address m_addr;
    size_t m_addr_index = 0;

    local_read_handle(global_data_view& storage, address&& addr)
        : m_storage(storage),
          m_addr(std::move(addr)) {}

    bool has_next() { return m_addr_index != m_addr.size(); }

    coro<void> next(std::vector<char>& buffer) {
        std::size_t buffer_size = 0;

        address partial_addr;
        while (m_addr_index < m_addr.size() and buffer_size < buffer.size()) {
            const auto frag = m_addr.get_fragment(m_addr_index);
            if (frag.size + buffer_size > buffer.size()) {
                break;
            }
            partial_addr.push_fragment(frag);
            buffer_size += frag.size;
            m_addr_index++;
        }

        co_await m_storage.read_address(buffer.data(), partial_addr);
        buffer.resize(buffer_size);
    }
};

/*
 * Extracted the following code into a function, as GCC will complain with
 * 'called on pointer returned from a mismatched allocation function'
 * otherwise (when number of co_awaits in get_object::handle() would be
 * greater than 5).
 */
coro<std::size_t> upload(local_read_handle& reader, http_request& req,
                         boost::asio::io_context& context) {
    size_t total_size = 0;

    std::shared_ptr<awaitable_promise<std::size_t>> promise;

    double_buffer buffers(MEBI_BYTE);
    while (reader.has_next()) {
        auto& buffer = buffers.current();
        buffer.resize(buffer.capacity());
        co_await reader.next(buffer);

        if (promise) {
            total_size += co_await promise->get();
        }

        promise = std::make_shared<awaitable_promise<std::size_t>>(context);

        boost::asio::async_write(req.socket(), boost::asio::buffer(buffer),
                                 use_awaitable_promise(promise));

        buffers.flip();
    }

    if (promise) {
        co_await promise->get();
        promise.reset();
    }

    co_return total_size;
}

} // namespace

get_object::get_object(const reference_collection& collection)
    : m_collection(collection) {}

bool get_object::can_handle(const http_request& req) {
    return req.method() == method::get && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("attributes");
}

coro<void> get_object::handle(http_request& req) const {
    metric<entrypoint_get_object_req>::increase(1);
    try {
        timer tt;

        auto obj = co_await m_collection.directory.get_object(req.bucket(),
                                                              req.object_key());

        http::response<http::empty_body> res{http::status::ok, 11};
        res.base().set("Content-Length", std::to_string(obj.size));

        http::response_serializer<http::empty_body> sr(res);
        co_await http::async_write_header(
            req.socket(), sr,
            boost::asio::as_tuple(boost::asio::use_awaitable));

        local_read_handle reader(m_collection.gdv, std::move(*obj.addr));

        size_t total_size = co_await upload(reader, req, m_collection.ioc);

        const std::chrono::duration<double> duration = tt.passed();
        const auto size = static_cast<double>(total_size) / MEBI_BYTE;
        const auto bandwidth = size / duration.count();

        metric<entrypoint_egressed_data_counter, byte>::increase(total_size);

        LOG_INFO() << "retrieval duration " << duration.count() << " s";
        LOG_INFO() << "retrieval bandwidth " << bandwidth << " MB/s";

    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }
}

} // namespace uh::cluster
