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

template <typename T>
struct owning_span {
    std::size_t size {0};
    std::unique_ptr <T[]> data = nullptr;
    owning_span() = default;
    explicit owning_span (size_t data_size):
            size (data_size),
            data {std::make_unique_for_overwrite <T[]> (size)} {}
    owning_span(size_t data_size, std::unique_ptr <T[]>&& ptr):
            size (data_size),
            data {std::move (ptr)} {}
    owning_span (owning_span&& os) noexcept: size (os.size), data (std::move (os.data)) {
        os.size = 0;
        os.data = nullptr;
    }
    void resize (std::size_t new_size) {
        size = new_size;
        data = std::make_unique_for_overwrite <T[]> (size);
    }
};

template <typename T>
using ospan = owning_span <T>;

struct dedupe_response {
    std::size_t effective_size {};
    address addr;
};

struct directory_put_request {
    std::string bucket_id;
    std::string object_key;
    address addr;
};

struct directory_get_request {
    std::string bucket_id;
    std::string object_key;
};

} // end namespace uh::cluster


#endif //CORE_COMMON_TYPES_H
