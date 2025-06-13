#pragma once

#include "big_int.h"
#include <cstdint>
#include <format>
#include <functional>
#include <iomanip>
#include <ostream>
#include <zpp_bits.h>

namespace uh::cluster {

struct fragment {
    uint128_t pointer;
    std::size_t size{};

    fragment() = default;

    fragment(uint128_t p, std::size_t s)
        : pointer(p),
          size(s) {}

    bool operator==(const fragment&) const = default;
    auto operator<=>(const fragment&) const = default;

    std::string to_string() const;

    /**
     * Return a fragment that spawns a sub range of this fragment.
     */
    fragment
    subfrag(std::size_t start,
            std::size_t end = std::numeric_limits<std::size_t>::max()) const;

    using serialize = zpp::bits::members<2>;
};

} // namespace uh::cluster

std::ostream& operator<<(std::ostream& os, const uh::cluster::fragment& frag);

template <>
struct std::formatter<uh::cluster::fragment> : std::formatter<std::string> {
    auto format(const uh::cluster::fragment& frag, std::format_context& ctx) {
        return std::formatter<std::string>::format(frag.to_string(), ctx);
    }
};

template <> struct std::hash<uh::cluster::fragment> {
    std::size_t operator()(const uh::cluster::fragment& obj) const noexcept {
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

namespace uh::cluster {

struct address {

    address() = default;

    /**
     * Construct address with given number of fragments, all set to zero.
     */
    explicit address(std::size_t size);

    auto operator<=>(const address&) const = default;

    /**
     * Push a fragment to the end of the address.
     */
    void push(const fragment& frag);

    /**
     * Push a fragment to the end of the address.
     */
    void emplace_back(uint128_t p, std::size_t s);

    /**
     * Get a fragment at a given index.
     */
    [[nodiscard]] fragment get(size_t i) const;

    /**
     * Append an address to this one.
     */
    void append(const address& addr);

    /**
     * Return amount of described data.
     */
    std::size_t data_size() const;

    /**
     * Return size of the address itself.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * Return true if the address is empty, ie. was default constructed.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * Return a sub range of the address.
     */
    address range(std::size_t start, std::size_t end) const;

    std::string to_string() const;

    /**
     * Return number of fragments for a given allocation size.
     */
    static constexpr std::size_t allocated_elements(std::size_t size) {
        return size / sizeof(fragment);
    }

    using serialize = zpp::bits::members<1>;

    std::vector<fragment> fragments;
};

inline std::vector<char> to_buffer(const address& addr) {
    std::vector<char> data;
    zpp::bits::out{data, zpp::bits::size4b{}}(addr).or_throw();
    return data;
}

inline address to_address(std::span<char> data) {
    address addr;
    zpp::bits::in{data, zpp::bits::size4b{}}(addr).or_throw();
    return addr;
}

} // namespace uh::cluster
