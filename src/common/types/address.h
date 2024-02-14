#ifndef UH_CLUSTER_ADDRESS_H
#define UH_CLUSTER_ADDRESS_H

#include "big_int.h"
#include "third-party/zpp_bits/zpp_bits.h"
#include <cstdint>

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
    explicit address(std::size_t size) : pointers(size * 2), sizes(size) {}
    explicit address(const fragment& frag) { push_fragment(frag); }

    void push_fragment(const fragment& frag) {
        pointers.emplace_back(frag.pointer.get_data()[0]);
        pointers.emplace_back(frag.pointer.get_data()[1]);
        sizes.emplace_back(frag.size);
    }

    void append_address(const address& addr) {
        pointers.insert(pointers.cend(), addr.pointers.cbegin(),
                        addr.pointers.cend());
        sizes.insert(sizes.cend(), addr.sizes.cbegin(), addr.sizes.cend());
    }

    void insert_fragment(int i, const fragment& frag) {
        pointers[2 * i] = frag.pointer.get_data()[0];
        pointers[2 * i + 1] = frag.pointer.get_data()[1];
        sizes[i] = frag.size;
    }

    void allocate_for_serialized_data(std::size_t size) {
        size_t count = size / (2 * sizeof(uint64_t) + sizeof(uint32_t));
        pointers.resize(count * 2);
        sizes.resize(count);
    }

    [[nodiscard]] fragment get_fragment(size_t i) const {
        return {{pointers[2 * i], pointers[2 * i + 1]}, sizes[i]};
    }

    void set_fragment(int i, const fragment& frag) {
        pointers[2 * i] = frag.pointer.get_high();
        pointers[2 * i + 1] = frag.pointer.get_low();
        sizes[i] = frag.size;
    }

    [[nodiscard]] std::size_t size() const noexcept { return sizes.size(); }

    [[nodiscard]] bool empty() const noexcept { return sizes.empty(); }

    // TODO: pop_front is not really cheap right now, perhaps this can be
    // revised

    fragment pop_front() {
        fragment frag = {{pointers[0], pointers[1]}, sizes[0]};

        pointers.erase(pointers.begin(), pointers.begin() + 2);
        sizes.erase(sizes.begin());

        return frag;
    }

    [[nodiscard]] fragment first() const {
        return {{pointers[0], pointers[1]}, sizes[0]};
    }

    using serialize = zpp::bits::members<2>;
    auto operator<=>(const address&) const = default;
};

} // namespace uh::cluster

template <> struct std::hash<uh::cluster::fragment> {
    std::size_t operator()(const uh::cluster::fragment& frag) const {
        return std::hash<std::uint64_t>{}(frag.pointer.get_high()) ^
               std::hash<std::uint64_t>{}(frag.pointer.get_low()) ^
               std::hash<std::size_t>{}(frag.size);
    }
};

#endif // UH_CLUSTER_ADDRESS_H
