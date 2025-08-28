/*
 * Abstract cache interface
 *
 * Cache interface support concurrent access. It handles data
 * structure only, not asynchronous tasks. I thought locking asynchronous tasks
 * using mutex is not a good idea.
 *
 * This is an interface for a cache that saves arbitrary sized objects as
 * values. So support size function is required for the value type. And all
 * cache implementations need to refer to the size for eviction decisions.
 */
#pragma once

#include <proxy/cache/entry.h>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace uh::cluster::proxy::cache {

namespace detail {

template <typename T>
concept has_size = requires(T t) {
    { t.size() } -> std::same_as<std::size_t>;
};

} // namespace detail

template <typename Entry>
concept EntryType = detail::has_size<Entry> && std::movable<Entry> &&
                    std::is_base_of_v<entry_interface<Entry>, Entry>;

template <typename Key, EntryType Entry> class cache_interface {
public:
    using timepoint_t = typename entry_interface<Entry>::time_point;

    virtual ~cache_interface() = default;

    [[nodiscard]] virtual std::shared_ptr<Entry> get(const Key& key) = 0;

    [[nodiscard]] virtual std::shared_ptr<Entry> remove(const Key& key) = 0;

    /*
     * Evict entries until the given size is freed. If size is 0, evict all
     */
    [[nodiscard]] virtual std::vector<std::shared_ptr<Entry>>
    evict(std::size_t size,
          std::optional<timepoint_t> expire_before = std::nullopt) = 0;

    /*
     * @return removed Entry if key exists, nullptr otherwise
     *
     * TODO: Let's add etag and expire_time as additional arguments
     */
    [[nodiscard]] virtual std::shared_ptr<Entry>
    put(const Key& key, std::shared_ptr<Entry> entry) = 0;

    virtual std::size_t size() const = 0;
};

} // namespace uh::cluster::proxy::cache
