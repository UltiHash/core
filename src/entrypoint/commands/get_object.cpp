#include "get_object.h"
#include "common/utils/time_utils.h"
#include "entrypoint/http/command_exception.h"
#include <entrypoint/http/range.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

class local_read_handle : public uh::cluster::ep::http::body {
public:
    local_read_handle(global_data_view& storage, directory::object_lock&& obj,
                      context& ctx)
        : m_storage(storage),
          m_obj(std::move(obj)),
          m_size(m_obj->addr->data_size()),
          m_ctx(ctx) {}

    ~local_read_handle() override {
        try {
            report_stats();
        } catch (...) {
        }
    }

    std::optional<std::size_t> length() const override { return m_size; }

    coro<std::size_t> read(std::span<char> buffer) override {
        std::size_t buffer_size = 0;

        address partial_addr;
        while (m_addr_index < m_obj->addr->size() and
               buffer_size < buffer.size()) {
            const auto frag = m_obj->addr->get(m_addr_index);
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
    directory::object_lock m_obj;
    size_t m_addr_index = 0;

    std::size_t m_size;
    context& m_ctx;

    std::size_t m_total = 0;
    timer m_timer;
};

} // namespace

get_object::get_object(directory& dir, global_data_view& storage)
    : m_dir(dir),
      m_storage(storage) {}

bool get_object::can_handle(const request& req) {
    return req.method() == verb::get && req.bucket() != RESERVED_BUCKET_NAME &&
           !req.bucket().empty() && !req.object_key().empty() &&
           !req.query("attributes");
}

coro<response> get_object::handle(request& req) {
    metric<entrypoint_get_object_req>::increase(1);

    response res;

    auto obj = co_await m_dir.get_object(req.bucket(), req.object_key());

    res.set("ETag", obj->etag);
    res.set("Content-Type", obj->mime);

    if (auto range = req.header("Range"); range) {
        res.base().result(status::partial_content);

        auto spec =
            ep::http::parse_range_header(*range, obj->addr->data_size());

        if (spec.ranges.size() != 1) {
            throw command_exception(status::not_implemented, "MultiRange",
                                    "no support for multiple ranges");
        }

        obj->addr = apply_range(*obj->addr, spec);
        LOG_DEBUG() << "range based access: header=" << *range;

        auto r = spec.ranges.front();
        r.end = r.start + obj->addr->data_size();

        res.set("Content-Range",
                "bytes " + r.to_string() + "/" + std::to_string(obj->size));
    }

    res.set_body(std::make_unique<local_read_handle>(m_storage, std::move(obj),
                                                     req.context()));

    co_return res;
}

std::string get_object::action_id() const { return "s3:GetObject"; }

} // namespace uh::cluster
