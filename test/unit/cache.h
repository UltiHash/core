#pragma once

#include <concepts>
#include <optional>

namespace uh::cluster {

template <typename Value>
concept has_size = requires(const Value& v) {
    { v.size() } -> std::convertible_to<std::size_t>;
};

template <typename Key, typename Value>
requires has_size<Value>
class cache_base {
public:
    virtual ~cache_base() = default;

    virtual std::optional<Value> access(const Key& key) = 0;
    virtual std::optional<Value> get(const Key& key) const = 0;
    virtual void put(const Key& key, const Value& value) = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t get_used_space() const = 0;
    virtual void clear() = 0;
};

} // namespace uh::cluster
