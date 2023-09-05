//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <vector>
#include <span>
#include <memory>
#include "address.h"

namespace uh::cluster {

//typedef boost::multiprecision::uint128_t uint128_t;


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
    FAILURE,
    STOP
};

enum role: uint8_t {
    DATA_NODE,
    DEDUPE_NODE,
    PHONE_BOOK_NODE,
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

} // end namespace uh::cluster


#endif //CORE_COMMON_H
