#include "get_object.h"

#include <common/utils/time_utils.h>

#include <entrypoint/formats.h>
#include <entrypoint/utils.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/http/range.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

namespace {

class local_read_handle : public uh::cluster::ep::http::body {
public:
    local_read_handle(storage::global::global_data_view& storage,
                      directory::object_lock&& obj,
                      std::size_t buffer_size = 32 * MEBI_BYTE)
        : m_storage(storage),
          m_obj(std::move(obj)),
          m_buffer(buffer_size),
          m_size(m_obj->addr->data_size()) {}

    ~local_read_handle() override {
        try {
            report_stats();
        } catch (...) {
        }
    }

    std::optional<std::size_t> length() const override { return m_size; }

    coro<std::span<const char>> read(std::size_t count) override {
        if (m_put_ptr - m_get_ptr < count) {
            co_await fill();
        }

        auto size = std::min(count, m_put_ptr - m_get_ptr);
        auto rv = std::span<const char>{ &m_buffer[m_get_ptr], size };

        m_get_ptr += size;

        co_return rv;
    }

    coro<std::span<const char>> read_until(std::string_view delimiter) {
        throw std::runtime_error("not implemented");
    }

    coro<void> fill() {
        std::size_t count = 0;

        address partial_addr;
        while (m_addr_index < m_obj->addr->size() && count + m_put_ptr < m_buffer.size()) {

            auto frag = m_obj->addr->get(m_addr_index);
            if (m_frag_offset > 0) {
                frag.pointer += m_frag_offset;
                frag.size -= m_frag_offset;
            }

            if (frag.size + count + m_put_ptr > m_buffer.size()) {
                auto remains = m_buffer.size() - (count + m_put_ptr);

                m_frag_offset += remains;
                frag.size = remains;
                partial_addr.push(frag);
                count += frag.size;
                break;
            }

            m_frag_offset = 0;
            partial_addr.push(frag);
            count += frag.size;
            m_addr_index++;
        }

        if (count > 0) {
            LOG_DEBUG() << "local_read_handle: fill, reading " << count << " bytes from storage";
            co_await m_storage.read_address(partial_addr, { &m_buffer[m_put_ptr], count });
            m_put_ptr += count;
            m_total += count;
            m_size -= count;
        }
    }

    coro<void> consume() override {
        auto count = m_put_ptr - m_get_ptr;
        if (count > 0) {
            LOG_DEBUG() << "local_read_handle: copying " << (m_put_ptr - m_get_ptr) << " bytes to new buffer";
            memmove(&m_buffer[0], &m_buffer[m_get_ptr], m_put_ptr - m_get_ptr);
            m_put_ptr -= m_get_ptr;
            m_get_ptr = 0;
        } else {
            m_put_ptr = m_get_ptr = 0ull;
        }

        co_return;
    }

    std::size_t buffer_size() const override { return m_buffer.size(); }
private:
    void report_stats() {
        metric<entrypoint_egressed_data_counter, byte>::increase(m_total);
    }

    storage::global::global_data_view& m_storage;
    directory::object_lock m_obj;

    std::vector<char> m_buffer;
    std::size_t m_get_ptr = 0ull;
    std::size_t m_put_ptr = 0ull;

    size_t m_addr_index = 0;
    std::size_t m_frag_offset = 0;

    std::size_t m_size;
    std::size_t m_total = 0;
};

} // namespace

get_object::get_object(directory& dir,
                       storage::global::global_data_view& storage)
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

    auto version = req.query("versionId");
    auto obj = co_await m_dir.get_object(req.bucket(), req.object_key(), version);

    if (version && (obj.empty() || obj->state == ep::object_state::deleted)) {
        res = error_response(
            status::method_not_allowed,
            "MethodNotAllowed",
            "The specified method is not allowed against this resource.");

        res.set("X-Amz-Delete-Marker", "true");
        if (!obj.empty()) {
            res.set("Last-Modified", imf_fixdate(obj->last_modified));
        }
        co_return res;
    }

    set_default_headers(res, *obj);

    if (auto range = req.header("Range"); range) {
        res.base().result(status::partial_content);

        auto spec =
            ep::http::parse_range_header(*range, obj->addr->data_size());

        if (spec.ranges.size() != 1) {
            throw command_exception(status::not_implemented, "NotImplemented",
                                    "No support for multiple ranges.");
        }

        obj->addr = apply_range(*obj->addr, spec);
        LOG_DEBUG() << req.peer() << ": range based access: header=" << *range;

        auto r = spec.ranges.front();
        r.end = r.start + obj->addr->data_size();

        res.set("Content-Range",
                "bytes " + r.to_string() + "/" + std::to_string(obj->size));
    }

    res.set_body(
        std::make_unique<local_read_handle>(m_storage, std::move(obj)));

    co_return res;
}

std::string get_object::action_id() const { return "s3:GetObject"; }

} // namespace uh::cluster
