#include "get_object.h"
#include "common/utils/time_utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

namespace {

class local_read_handle : public uh::cluster::body {
public:
    local_read_handle(global_data_view& storage, address&& addr, context& ctx)
        : m_storage(storage),
          m_addr(std::move(addr)),
          m_size(m_addr.data_size()),
          m_ctx(ctx) {}

    ~local_read_handle() {
        try {
            report_stats();
        } catch (...) {
        }
    }

    std::optional<std::size_t> length() const override { return m_size; }

    coro<std::size_t> read(std::span<char> buffer) override {
        std::size_t buffer_size = 0;

        address partial_addr;
        while (m_addr_index < m_addr.size() and buffer_size < buffer.size()) {
            const auto frag = m_addr.get(m_addr_index);
            if (frag.size + buffer_size > buffer.size()) {
                break;
            }
            partial_addr.push(frag);
            buffer_size += frag.size;
            m_addr_index++;
        }

        co_await m_storage.read_address(m_ctx, buffer.data(), partial_addr);
        m_total += buffer_size;
        m_size -= buffer_size;
        co_return buffer_size;
    }

private:
    void report_stats() {
        const std::chrono::duration<double> duration = m_timer.passed();
        const auto size = static_cast<double>(m_total) / MEBI_BYTE;
        const auto bandwidth = size / duration.count();

        metric<entrypoint_egressed_data_counter, byte>::increase(m_total);

        LOG_INFO() << "retrieval duration " << duration.count() << " s";
        LOG_INFO() << "retrieval bandwidth " << bandwidth << " MB/s";
    }

    global_data_view& m_storage;
    const address m_addr;
    size_t m_addr_index = 0;

    std::size_t m_size;
    context& m_ctx;

    std::size_t m_total = 0;
    timer m_timer;
};

} // namespace

get_object::get_object(const reference_collection& collection)
    : m_collection(collection) {}

bool get_object::can_handle(const http_request& req) {
    return req.method() == method::get &&
           req.bucket() != RESERVED_BUCKET_NAME && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("attributes");
}

coro<http_response> get_object::handle(http_request& req) const {
    metric<entrypoint_get_object_req>::increase(1);

    http_response res;

    try {
        auto obj = co_await m_collection.directory.get_object(req.bucket(),
                                                              req.object_key());

        res.set("ETag", obj.etag);
        res.set("Content-Type", obj.mime);
        res.set_body(std::make_unique<local_read_handle>(
            m_collection.gdv, std::move(*obj.addr), req.m_ctx));
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }

    co_return res;
}

} // namespace uh::cluster
