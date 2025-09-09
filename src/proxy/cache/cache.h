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

#include <chrono>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace uh::cluster::proxy::cache {

namespace detail {

template <typename T>
concept has_data_size = requires(T t) {
    { t.data_size() } -> std::same_as<std::size_t>;
};

} // namespace detail

template <typename Entry>
concept EntryType = detail::has_data_size<Entry> && std::movable<Entry>;

template <typename Key, EntryType Entry> class cache_interface {
public:
    virtual ~cache_interface() = default;

    [[nodiscard]] virtual std::shared_ptr<Entry> get(const Key& key) = 0;

    [[nodiscard]] virtual std::shared_ptr<Entry> remove(const Key& key) = 0;

    /*
     * Evict entries until the given size is freed
     */
    [[nodiscard]] virtual std::vector<std::shared_ptr<Entry>>
    evict(std::size_t size) = 0;

    /*
     * @param key key of the entry
     * @param entry entry to be inserted or updated (used rvalue reference
     * instead shared_ptr, since some cache implementation may need to create
     * local_entry using it)
     *
     * @return removed Entry if key exists, nullptr otherwise
     */
    [[nodiscard]] virtual std::shared_ptr<Entry> put(const Key& key,
                                                     Entry&& entry) = 0;

    virtual std::size_t size() const = 0;
};

} // namespace uh::cluster::proxy::cache
