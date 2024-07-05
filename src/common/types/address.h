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
};

struct address {

    std::vector<uint64_t> pointers;
    std::vector<uint32_t> sizes;

    address() = default;
    explicit address(std::size_t size);
    explicit address(const fragment& frag);

    address shrink() const;

    void push_fragment(const fragment& frag);

    void append_address(const address& addr);

    void insert_fragment(int i, const fragment& frag);

    void allocate_for_serialized_data(std::size_t size);

    std::size_t data_size() const;

    [[nodiscard]] fragment get_fragment(size_t i) const;

    void set_fragment(int i, const fragment& frag);

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] fragment first() const;

    using serialize = zpp::bits::members<2>;
    auto operator<=>(const address&) const = default;
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
