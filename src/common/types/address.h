#pragma once

#include "fragment.h"

#include <common/types/address.h>
#include <cstdint>
#include <format>
#include <functional>
#include <iomanip>
#include <ostream>

namespace uh::cluster {

template <FragmentPointer T = uint128_t> struct address_t {

    address_t() = default;

    /**
     * Construct address with given number of fragments, all set to zero.
     */
    explicit address_t(std::size_t size)
        : fragments(size) {}

    auto operator<=>(const address_t<T>&) const = default;

    /**
     * Push a fragment to the end of the address.
     */
    void push(const fragment_t<T>& frag) { fragments.push_back(frag); }

    /**
     * Push a fragment to the end of the address.
     */
    void emplace_back(uint128_t p, std::size_t s) {
        fragments.emplace_back(p, s);
    }

    /**
     * Get a fragment at a given index.
     */
    [[nodiscard]] fragment_t<T> get(size_t i) const { return fragments[i]; }

    /**
     * Append an address to this one.
     */
    void append(const address_t<T>& other) {
        fragments.insert(fragments.cend(), other.fragments.cbegin(),
                         other.fragments.cend());
    }

    /**
     * Return amount of described data.
     */
    std::size_t data_size() const {
        return std::accumulate(fragments.begin(), fragments.end(), 0ull,
                               [](std::size_t acc, const fragment_t<T>& f) {
                                   return acc + f.size;
                               });
    }

    /**
     * Return size of the address itself.
     */
    [[nodiscard]] std::size_t size() const noexcept { return fragments.size(); }

    /**
     * Return true if the address is empty, ie. was default constructed.
     */
    [[nodiscard]] bool empty() const noexcept { return fragments.empty(); }

    /**
     * Return a sub range of the address.
     */
    address_t<T> range(std::size_t start, std::size_t end) const {
        fragment_t<T> f;

        std::size_t ptr = 0ull;
        std::size_t index = 0ull;
        for (; index < size(); ++index) {
            f = get(index);
            if (start < ptr + f.size) {
                f = f.subfrag(start - ptr, end - ptr);
                ptr = start + f.size;
                break;
            }

            ptr += f.size;
        }

        if (index == size()) {
            return {};
        }

        address_t<T> rv;
        rv.push(f);

        for (++index; ptr < end && index < size(); ++index) {
            f = get(index).subfrag(0, end - ptr);
            rv.push(f);
            ptr += f.size;
        }

        return rv;
    }

    std::string to_string() const {
        std::string frags;

        for (auto f : fragments) {
            if (!frags.empty()) {
                frags += ", ";
            }

            frags += f.to_string();
        }

        return frags;
    }

    /**
     * Return number of fragments for a given allocation size.
     */
    static constexpr std::size_t allocated_elements(std::size_t size) {
        return size / sizeof(fragment_t<T>);
    }

    using serialize = zpp::bits::members<1>;

    std::vector<fragment_t<T>> fragments;
};

using pointer = uint128_t;
using storage_pointer = uint64_t;

using fragment = fragment_t<pointer>;
using address = address_t<pointer>;
using storage_fragment = fragment_t<storage_pointer>;
using storage_address = address_t<storage_pointer>;

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
