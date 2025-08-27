#pragma once

#include <proxy/cache/storage_body.h>

#include <common/types/address.h>
#include <common/types/common_types.h>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>
#include <entrypoint/utils.h>
#include <storage/global/data_view.h>
#include <string>

namespace uh::cluster {

struct object_metadata {
    std::string path;
    std::string version;

    bool operator==(const object_metadata& other) const {
        return path == other.path && version == other.version;
    }
};

} // namespace uh::cluster

template <> struct std::hash<uh::cluster::object_metadata> {
    size_t operator()(const uh::cluster::object_metadata& key) const {
        std::size_t seed = 0;

        auto hash_combine = [](std::size_t& seed, const auto& v) {
            seed ^= std::hash<std::remove_cvref_t<decltype(v)>>{}(v) +
                    0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

        hash_combine(seed, key.path);
        hash_combine(seed, key.version);

        return seed;
    }
};

namespace uh::cluster {

class object_handle {
public:
    using global_data_view = storage::global::global_data_view;
    using stream = ep::http::stream;

    std::size_t size() const { return m_addr.data_size(); }

    object_handle(object_handle&&) = default;
    object_handle& operator=(object_handle&&) = default;

    ~object_handle() {
        if (!m_deleted) {
            // TODO: log warning
        }
    }

    const address& addr() { return m_addr; }

    static coro<object_handle> create(ep::http::storage_writer_body& b) {
        co_await read(b);
        co_return object_handle{
            std::move(b.get_dedupe_response().addr),
        };
    }

    ep::http::storage_reader_body get_reader_body(stream s) {
        return ep::http::storage_reader_body{s, m_reader.get(), m_addr};
    }

    coro<void> destroy(global_data_view& data_view) {
        auto s = co_await m_reader.get().unlink(m_addr);
        if (s != size()) {
            throw std::runtime_error("failed to delete object");
        }
        m_deleted = true;
    }

private:
    address m_addr;
    // TODO: Add etag here
    bool m_deleted{false};

    object_handle(address addr)
        : m_addr(std::move(addr)) {}
};

} // namespace uh::cluster
