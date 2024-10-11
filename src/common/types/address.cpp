#include "address.h"

namespace uh::cluster {

std::string fragment::to_string() const {
    return pointer.to_string() + "[" + std::to_string(size) + "]";
}

fragment fragment::subfrag(std::size_t start, std::size_t end) const {
    if (start >= end) {
        return {};
    }

    return {pointer + start, std::min(size, end) - start};
}

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

address address::range(std::size_t start, std::size_t end) const {
    fragment f;

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

    address rv;
    rv.push(f);

    for (++index; ptr < end && index < size(); ++index) {
        f = get(index).subfrag(0, end - ptr);
        rv.push(f);
        ptr += f.size;
    }

    return rv;
}

std::string address::to_string() const {
    std::string frags;

    for (std::size_t index = 0; index < sizes.size(); ++index) {
        if (!frags.empty()) {
            frags += ", ";
        }

        frags += get(index).to_string();
    }

    return frags;
}

} // namespace uh::cluster
