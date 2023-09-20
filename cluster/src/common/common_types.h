//
// Created by masi on 9/6/23.
//

#ifndef CORE_COMMON_TYPES_H
#define CORE_COMMON_TYPES_H

#include <vector>
#include <span>
#include <third-party/zpp_bits/zpp_bits.h>
#include "big_int.h"

namespace uh::cluster {

typedef big_int uint128_t;

struct fragment {
    uint128_t pointer;
    size_t size {};
};

struct address {

    std::vector <uint64_t> pointers;
    std::vector <uint16_t> sizes;

    address () = default;
    explicit address (std::size_t size):
            pointers (size * 2),
            sizes (size) {}
    explicit address (const fragment& frag) {
        push_fragment(frag);
    }

    void push_fragment (const fragment& frag) {
        pointers.emplace_back(frag.pointer.get_data()[0]);
        pointers.emplace_back(frag.pointer.get_data()[1]);
        sizes.emplace_back(frag.size);
    }

    void append_address (const address& addr) {
        pointers.insert(pointers.cend(), addr.pointers.cbegin(), addr.pointers.cend());
        sizes.insert(sizes.cend(), addr.sizes.cbegin(), addr.sizes.cend());
    }

    void insert_fragment (int i, const fragment& frag) {
        pointers [2*i] = frag.pointer.get_data()[0];
        pointers [2*i + 1] = frag.pointer.get_data()[1];
        sizes [i] = frag.size;
    }

    void allocate_for_serialized_data (std::size_t size) {
        size_t count = size / (2 * sizeof (uint64_t) + sizeof (uint16_t));
        pointers.resize (count * 2);
        sizes.resize (count);
    }

    [[nodiscard]] fragment get_fragment (int i) const {
        return {{pointers[2*i], pointers[2*i+1]}, sizes[i]};
    }

    [[nodiscard]] std::size_t size () const noexcept {
        return sizes.size();
    }

    using serialize = zpp::bits::members<2>;
    auto operator<=>(const address&) const = default;
};

struct dedupe_response {
    std::size_t effective_size {};
    address addr;
};

struct directory_put_request {
    std::string bucket_id;
    std::string object_key;
    address addr;

    auto operator<=>(const directory_put_request&) const = default;
};

struct directory_get_request {
    std::string bucket_id;
    std::string object_key;
};

} // end namespace uh::cluster


#endif //CORE_COMMON_TYPES_H
