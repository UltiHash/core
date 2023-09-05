//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <vector>
#include <span>
#include "big_int.h"
#include <memory>

namespace uh::cluster {

//typedef boost::multiprecision::uint128_t uint128_t;

typedef big_int uint128_t;

struct fragment {
    uint128_t pointer;
    size_t size {};
};

//typedef std::vector <wide_span> address;

enum message_types:uint8_t {
    INIT_REQ,
    INIT_RESP,
    READ_REQ,
    READ_RESP,
    WRITE_REQ,
    WRITE_RESP,
    WRITE_MANY_REQ,
    WRITE_MANY_RESP,
    SYNC_REQ,
    SYNC_OK,
    REMOVE_REQ,
    REMOVE_OK,
    USED_REQ,
    USED_RESP,
    DEDUPE_REQ,
    DEDUPE_RESP,
    DIR_PUT_OBJ_REQ,
    DIR_PUT_OBJ_RESP,
    FAILURE,
    STOP
};

enum role: uint8_t {
    DATA_NODE,
    DEDUPE_NODE,
    DIRECTORY_NODE,
    ENTRY_NODE,
};

void* align_ptr (void* ptr) noexcept;
void sync_ptr (void *ptr, std::size_t size);

template <typename T>
struct owning_span {
    std::size_t size {};
    std::unique_ptr <T[]> data = nullptr;
    owning_span() = default;
    explicit owning_span (size_t data_size):
            size (data_size),
            data {std::make_unique_for_overwrite <T[]> (size)} {}
    owning_span(size_t data_size, std::unique_ptr <T[]>&& ptr):
            size (data_size),
            data {std::move (ptr)} {}
    void resize (std::size_t new_size) {
        data = std::make_unique_for_overwrite <T[]> (size);
        size = new_size;
    }
};

template <typename T>
using ospan = owning_span <T>;

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
};


} // end namespace uh::cluster


#endif //CORE_COMMON_H
