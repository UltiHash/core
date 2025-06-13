#include "address.h"

#include <common/utils/pointer_traits.h>
#include <format>

namespace uh::cluster {

std::string fragment::to_string() const {
    return std::format("[group {}, pointer {}, size {}]",
                       pointer_traits::get_group_id(pointer),
                       pointer_traits::get_group_pointer(pointer), size);
}

} // namespace uh::cluster

std::ostream& operator<<(std::ostream& os, const uh::cluster::fragment& frag) {
    return os << frag.to_string();
}

namespace uh::cluster {

fragment fragment::subfrag(std::size_t start, std::size_t end) const {
    if (start >= end) {
        return {};
    }

    return {pointer + start, std::min(size, end) - start};
}

address::address(std::size_t size)
    : fragments(size) {}

void address::push(const fragment& frag) { fragments.push_back(frag); }

void address::emplace_back(uint128_t p, std::size_t s) {
    fragments.emplace_back(p, s);
}

void address::append(const address& other) {
    fragments.insert(fragments.cend(), other.fragments.cbegin(),
                     other.fragments.cend());
}

std::size_t address::data_size() const {
    return std::accumulate(
        fragments.begin(), fragments.end(), 0ull,
        [](std::size_t acc, const fragment& f) { return acc + f.size; });
}

[[nodiscard]] fragment address::get(size_t i) const { return fragments[i]; }

[[nodiscard]] std::size_t address::size() const noexcept {
    return fragments.size();
}

[[nodiscard]] bool address::empty() const noexcept { return fragments.empty(); }

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

    for (auto f : fragments) {
        if (!frags.empty()) {
            frags += ", ";
        }

        frags += f.to_string();
    }

    return frags;
}

} // namespace uh::cluster
