//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <vector>
#include <span>
#include <memory>
#include "common_types.h"

namespace uh::cluster {

//typedef boost::multiprecision::uint128_t uint128_t;


//typedef std::vector <wide_span> address;

enum message_types:uint8_t {
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
    DIR_GET_OBJ_REQ,
    DIR_GET_OBJ_RESP,
    SUCCESS,
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


} // end namespace uh::cluster


#endif //CORE_COMMON_H
