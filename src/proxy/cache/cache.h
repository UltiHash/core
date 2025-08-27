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
#include <string>
#include <vector>

namespace uh::cluster {

namespace detail {

template <typename T>
concept has_size = requires(T t) {
    { t.size() } -> std::same_as<std::size_t>;
};

} // namespace detail

template <typename Key, typename Value>
requires detail::has_size<Value> && std::movable<Value>
class cache {
public:
    using time_point = std::chrono::system_clock::time_point;
    struct entry {
        Value value;

        // TODO: Make expiration duration configurable
        entry(Value v)
            : value(std::move(v)),
              expire_time{std::chrono::system_clock::now() +
                          std::chrono::seconds(10)} {}
        // TODO: Let's add expire_time as member variables and corresponding
        // interfaces
        //
        // bool is_fresh();
        // vod revive();
        time_point get_expire_time() const { return expire_time; }

    private:
        time_point expire_time;
    };
    virtual ~cache() = default;

    [[nodiscard]] virtual std::shared_ptr<entry> get(const Key& key) = 0;

    [[nodiscard]] virtual std::shared_ptr<entry> remove(const Key& key) = 0;

    /*
     * Evict entries until the given size is freed. If size is 0, evict all
     */
    [[nodiscard]] virtual std::vector<std::shared_ptr<entry>>
    evict(std::size_t size,
          std::optional<time_point> expire_before = std::nullopt) = 0;

    /*
     * @return removed entry if key exists, nullptr otherwise
     *
     * TODO: Let's add etag and expire_time as additional arguments
     */
    [[nodiscard]] virtual std::shared_ptr<entry> put(const Key& key,
                                                     Value value) = 0;

    virtual std::size_t size() const = 0;
};

} // namespace uh::cluster
