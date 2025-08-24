#pragma once

#include <concepts>
#include <optional>

namespace uh::cluster {

namespace detail {

template <typename T> std::size_t get_size(T&& obj) {
    if constexpr (requires { obj.size(); }) {
        return obj.size();
    } else if constexpr (requires { (*obj).size(); }) {
        return (*obj).size();
    } else {
        static_assert(sizeof(T) == 0, "Type does not support size()");
    }
}

template <typename T>
concept size_computable_t = requires(T t) {
    { get_size(t) } -> std::convertible_to<std::size_t>;
};

} // namespace detail

template <typename Key, typename Value>
requires detail::size_computable_t<Value> && std::copyable<Value>
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
