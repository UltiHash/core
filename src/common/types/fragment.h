#pragma once

#include "big_int.h"

#include <zpp_bits.h>

namespace uh::cluster {

template <typename T>
concept FragmentPointer =
    std::is_same_v<T, uint128_t> || std::is_same_v<T, uint64_t>;

template <FragmentPointer T = uint128_t> struct fragment_t {
    T pointer;
    std::size_t size{};

    fragment_t() = default;

    fragment_t(T p, std::size_t s)
        : pointer(p),
          size(s) {}

    bool operator==(const fragment_t&) const = default;
    auto operator<=>(const fragment_t&) const = default;

    std::string to_string() const;

    /**
     * Return a fragment that spawns a sub range of this fragment.
     */
    fragment_t
    subfrag(std::size_t start,
            std::size_t end = std::numeric_limits<std::size_t>::max()) const {
        if (start >= end) {
            return {};
        }

        return {pointer + start, std::min(size, end) - start};
    }

    using serialize = zpp::bits::members<2>;
};

} // namespace uh::cluster

template <uh::cluster::FragmentPointer T>
std::ostream& operator<<(std::ostream& os,
                         const uh::cluster::fragment_t<T>& frag) {
    return os << frag.to_string();
}

template <uh::cluster::FragmentPointer T>
struct std::formatter<uh::cluster::fragment_t<T>>
    : std::formatter<std::string> {
    auto format(const uh::cluster::fragment_t<T>& frag,
                std::format_context& ctx) {
        return std::formatter<std::string>::format(frag.to_string(), ctx);
    }
};

template <> struct std::hash<uh::cluster::fragment_t<uint128_t>> {
    std::size_t
    operator()(const uh::cluster::fragment_t<uint128_t>& obj) const noexcept {
        uint64_t high = static_cast<uint64_t>(obj.pointer >> 64);
        uint64_t low = static_cast<uint64_t>(obj.pointer);

        std::size_t hash_high = std::hash<uint64_t>{}(high);
        std::size_t hash_low = std::hash<uint64_t>{}(low);
        std::size_t hash_size = std::hash<std::size_t>{}(obj.size);

        std::size_t seed = hash_high;
        auto hash_combine = [&](std::size_t& s, std::size_t v) {
            s ^= v + 0x9e3779b97f4a7c16ULL + (s << 6) + (s >> 2);
        };
        hash_combine(seed, hash_low);
        hash_combine(seed, hash_size);

        return seed;
    }
};

template <> struct std::hash<uh::cluster::fragment_t<uint64_t>> {
    std::size_t
    operator()(const uh::cluster::fragment_t<uint64_t>& obj) const noexcept {
        std::size_t hash_pointer = std::hash<uint64_t>{}(obj.pointer);
        std::size_t hash_size = std::hash<std::size_t>{}(obj.size);

        std::size_t seed = hash_pointer;
        auto hash_combine = [&](std::size_t& s, std::size_t v) {
            s ^= v + 0x9e3779b97f4a7c16ULL + (s << 6) + (s >> 2);
        };
        hash_combine(seed, hash_size);

        return seed;
    }
};
