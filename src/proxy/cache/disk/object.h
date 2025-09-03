#pragma once

#include <proxy/cache/entry.h>

#include <common/types/address.h>
#include <string>

namespace uh::cluster::proxy::cache::disk {

struct object_metadata {
    std::string path;
    std::string version;

    bool operator==(const object_metadata& other) const {
        return path == other.path && version == other.version;
    }
};

} // namespace uh::cluster::proxy::cache::disk

template <> struct std::hash<uh::cluster::proxy::cache::disk::object_metadata> {
    size_t operator()(
        const uh::cluster::proxy::cache::disk::object_metadata& key) const {
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

namespace uh::cluster::proxy::cache::disk {

struct object_handle {
    object_handle() = default;
    object_handle(address&& a, std::string etag = "",
                  std::chrono::seconds ttl = std::chrono::seconds(0))
        : m_addr(std::move(a)),
          m_etag{std::move(etag)},
          m_expire_at{std::chrono::steady_clock::now() + ttl} {}

    object_handle(object_handle&&) = default;
    object_handle& operator=(object_handle&&) = default;

    std::size_t data_size() const { return m_addr.data_size(); }

    const address& get_address() const { return m_addr; }

    const std::string& get_etag() const { return m_etag; }

    bool is_expired() const {
        return std::chrono::steady_clock::now() >= m_expire_at;
    }

    void refresh(std::chrono::seconds ttl) {
        m_expire_at = std::chrono::steady_clock::now() + ttl;
    }

private:
    address m_addr;
    std::string m_etag;
    std::chrono::steady_clock::time_point m_expire_at;
};

} // namespace uh::cluster::proxy::cache::disk
