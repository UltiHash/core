#ifndef UH_CLUSTER_ADDRESS_H
#define UH_CLUSTER_ADDRESS_H

#include "big_int.h"
#include <cstdint>
#include <zpp_bits.h>

namespace uh::cluster {

struct fragment {
    uint128_t pointer;
    size_t size{};

    bool operator==(const fragment& frag) const {
        return pointer == frag.pointer && size == frag.size;
    }

    std::string to_string() const;

    /**
     * Return a fragment that spawns a sub range of this fragment.
     */
    fragment
    subfrag(std::size_t start,
            std::size_t end = std::numeric_limits<std::size_t>::max()) const;
};

struct address {

    address() = default;

    /**
     * Construct address with given number of fragments, all set to zero.
     */
    explicit address(std::size_t size);

    auto operator<=>(const address&) const = default;

    /**
     * Return an address describing the same buffer but with adjacent fragments
     * merged.
     */
    address shrink() const;

    /**
     * Push a fragment to the end of the address.
     */
    void push(const fragment& frag);

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
        return size / (2 * sizeof(uint64_t) + sizeof(uint32_t));
    }

    using serialize = zpp::bits::members<2>;

    std::vector<uint64_t> pointers;
    std::vector<uint32_t> sizes;
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

template <> struct std::hash<uh::cluster::fragment> {
    std::size_t operator()(const uh::cluster::fragment& frag) const {
        return std::hash<std::uint64_t>{}(frag.pointer.get_high()) ^
               std::hash<std::uint64_t>{}(frag.pointer.get_low()) ^
               std::hash<std::size_t>{}(frag.size);
    }
};

#endif // UH_CLUSTER_ADDRESS_H
