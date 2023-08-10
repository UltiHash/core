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

struct wide_span {
    uint128_t pointer;
    size_t size {};
};

typedef std::vector <wide_span> address;

enum message_types {
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

void handle_failure (const std::string& job_name, int target, const std::exception &e);

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

};

template <typename T>
using ospan = owning_span <T>;
} // end namespace uh::cluster



#endif //CORE_COMMON_H
