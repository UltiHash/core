#include "address.h"

namespace uh::cluster {

address::address(std::size_t size)
    : pointers(size * 2),
      sizes(size) {}

address::address(const fragment& frag) { push_fragment(frag); }

address address::shrink() const {
    address rv;

    if (empty()) {
        return rv;
    }

    uint128_t ptr(pointers[0], pointers[1]);
    std::size_t size = sizes[0];

    for (std::size_t index = 1; index < sizes.size(); ++index) {

        uint128_t current(pointers[2 * index], pointers[2 * index + 1]);

        if (ptr + size == current) {
            size += sizes[index];
            continue;
        }

        rv.push_fragment({ptr, size});
        ptr = current;
        size = sizes[index];
    }

    rv.push_fragment({ptr, size});

    return rv;
}

void address::push_fragment(const fragment& frag) {
    pointers.emplace_back(frag.pointer.get_data()[0]);
    pointers.emplace_back(frag.pointer.get_data()[1]);
    sizes.emplace_back(frag.size);
}

void address::append_address(const address& addr) {
    pointers.insert(pointers.cend(), addr.pointers.cbegin(),
                    addr.pointers.cend());
    sizes.insert(sizes.cend(), addr.sizes.cbegin(), addr.sizes.cend());
}

void address::insert_fragment(int i, const fragment& frag) {
    pointers[2 * i] = frag.pointer.get_data()[0];
    pointers[2 * i + 1] = frag.pointer.get_data()[1];
    sizes[i] = frag.size;
}

void address::allocate_for_serialized_data(std::size_t size) {
    size_t count = size / (2 * sizeof(uint64_t) + sizeof(uint32_t));
    pointers.resize(count * 2);
    sizes.resize(count);
}

std::size_t address::data_size() const {
    return std::accumulate(sizes.begin(), sizes.end(), 0ull);
}

[[nodiscard]] fragment address::get_fragment(size_t i) const {
    return {{pointers[2 * i], pointers[2 * i + 1]}, sizes[i]};
}

void address::set_fragment(int i, const fragment& frag) {
    pointers[2 * i] = frag.pointer.get_high();
    pointers[2 * i + 1] = frag.pointer.get_low();
    sizes[i] = frag.size;
}

[[nodiscard]] std::size_t address::size() const noexcept {
    return sizes.size();
}

[[nodiscard]] bool address::empty() const noexcept { return sizes.empty(); }

[[nodiscard]] fragment address::first() const {
    return {{pointers[0], pointers[1]}, sizes[0]};
}

} // namespace uh::cluster
