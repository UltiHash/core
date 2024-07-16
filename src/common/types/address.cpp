#include "address.h"

namespace uh::cluster {

address::address(std::size_t size)
    : pointers(size * 2),
      sizes(size) {}

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

        rv.push({ptr, size});
        ptr = current;
        size = sizes[index];
    }

    rv.push({ptr, size});

    return rv;
}

void address::push(const fragment& frag) {
    pointers.emplace_back(frag.pointer.get_data()[0]);
    pointers.emplace_back(frag.pointer.get_data()[1]);
    sizes.emplace_back(frag.size);
}

void address::append(const address& addr) {
    pointers.insert(pointers.cend(), addr.pointers.cbegin(),
                    addr.pointers.cend());
    sizes.insert(sizes.cend(), addr.sizes.cbegin(), addr.sizes.cend());
}

std::size_t address::data_size() const {
    return std::accumulate(sizes.begin(), sizes.end(), 0ull);
}

[[nodiscard]] fragment address::get(size_t i) const {
    return {{pointers[2 * i], pointers[2 * i + 1]}, sizes[i]};
}

[[nodiscard]] std::size_t address::size() const noexcept {
    return sizes.size();
}

[[nodiscard]] bool address::empty() const noexcept { return sizes.empty(); }

} // namespace uh::cluster
