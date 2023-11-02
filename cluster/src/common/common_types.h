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

    bool operator==(const fragment& frag) const {
        return pointer == frag.pointer && size == frag.size;
    }
};

struct address {

    std::vector <uint64_t> pointers;
    std::vector <uint32_t> sizes;

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
        int i = 0;
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
        size_t count = size / (2 * sizeof (uint64_t) + sizeof (uint32_t));
        pointers.resize (count * 2);
        sizes.resize (count);
    }

    [[nodiscard]] fragment get_fragment (int i) const {
        return {{pointers[2*i], pointers[2*i+1]}, sizes[i]};
    }

    void set_fragment(int i, const fragment& frag) {
        pointers[2*i] = frag.pointer.get_high();
        pointers[2*i+1] = frag.pointer.get_low();
        sizes[i] = frag.size;
    }

    [[nodiscard]] std::size_t size () const noexcept {
        return sizes.size();
    }

    [[nodiscard]] bool empty () const noexcept {
        return sizes.empty();
    }

    // TODO: pop_front is not really cheap right now, perhaps this can be revised

    fragment pop_front() {
        fragment frag = { {pointers[0], pointers[1]}, sizes[0]};

        pointers.erase(pointers.begin(), pointers.begin()+2);
        sizes.erase(sizes.begin());

        return frag;
    }

    [[nodiscard]] fragment first() const {
        return {{pointers[0], pointers[1]}, sizes[0]};
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

struct directory_message {
    std::string bucket_id;
    zpp::bits::optional_ptr <std::string> object_key;
    zpp::bits::optional_ptr <std::string> object_key_prefix;
    zpp::bits::optional_ptr <std::string> object_key_lower_bound;
    zpp::bits::optional_ptr<address> addr;

    bool operator ==(const directory_message& rhs) const {
        bool result = bucket_id == rhs.bucket_id;

        if(object_key.get() != nullptr && rhs.object_key.get() != nullptr)
            result &= *object_key == *rhs.object_key;
        else if(object_key.get() == nullptr && rhs.object_key.get() == nullptr)
            result &= true;
        else
            return false;

        if(object_key_prefix.get() != nullptr && rhs.object_key_prefix.get() != nullptr)
            result &= *object_key_prefix == *rhs.object_key_prefix;
        else if(object_key_prefix.get() == nullptr && rhs.object_key_prefix.get() == nullptr)
            result &= true;
        else
            return false;

        if(object_key_lower_bound.get() != nullptr && rhs.object_key_lower_bound.get() != nullptr)
            result &= *object_key_lower_bound == *rhs.object_key_lower_bound;
        else if(object_key_lower_bound.get() == nullptr && rhs.object_key_lower_bound.get() == nullptr)
            result &= true;
        else
            return false;

        if(addr.get() != nullptr && rhs.addr.get() != nullptr)
            result &= *addr == *rhs.addr;
        else if(addr.get() == nullptr && rhs.addr.get() == nullptr)
            result &= true;
        else
            return false;

        return result;

    };
};

struct directory_lst_entities_message {
    std::vector<std::string> entities;
};

struct allocated_write_message {
    address addr;
    std::variant <std::string_view, ospan <char>> data;
};

} // end namespace uh::cluster

template<> struct std::hash<uh::cluster::fragment> {
    std::size_t operator()(const uh::cluster::fragment& frag) const {
        return std::hash<std::uint64_t>{}(frag.pointer.get_high())
             ^ std::hash<std::uint64_t>{}(frag.pointer.get_low())
             ^ std::hash<std::size_t>{}(frag.size);
    }
};


#endif //CORE_COMMON_TYPES_H
