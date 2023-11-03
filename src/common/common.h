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

enum message_type: uint8_t {
    READ_REQ = 10,
    READ_RESP = 11,
    WRITE_REQ = 12,
    WRITE_RESP = 13,
    ALLOC_REQ = 14,
    ALLOC_RESP = 15,
    DEALLOC_REQ = 16,
    DEALLOC_RESP = 17,
    ALLOC_WRITE_REQ = 18,
    ALLOC_WRITE_RESP = 19,
    SYNC_REQ = 20,
    SYNC_OK = 21,
    REMOVE_REQ = 22,
    REMOVE_OK = 23,
    USED_REQ = 24,
    USED_RESP = 25,
    DEDUPE_REQ = 26,
    DEDUPE_RESP = 27,
    DIR_PUT_OBJ_REQ = 28,
    DIR_GET_OBJ_REQ = 29,
    DIR_GET_OBJ_RESP = 30,
    DIR_PUT_BUCKET_REQ = 31,
    SUCCESS = 32,
    FAILURE = 33,
    STOP = 34,
    RECOVER_REQ = 35,
    RECOVER_RESP = 36,
    DIR_LIST_BUCKET_REQ = 37,
    DIR_LIST_BUCKET_RESP = 38,
    DIR_LIST_OBJ_REQ = 39,
    DIR_LIST_OBJ_RESP = 40,
    DIR_DELETE_BUCKET_REQ = 41,
    DIR_DELETE_BUCKET_RESP = 42,
    READ_ADDRESS_REQ = 43,
    READ_ADDRESS_RESP = 44,
};

enum role: uint8_t {
    DATA_NODE,
    DEDUPE_NODE,
    DIRECTORY_NODE,
    ENTRY_NODE,
};

enum ec_type: uint8_t {
    NON = 0,
    XOR,
};

void* align_ptr (void* ptr) noexcept;
void sync_ptr (void *ptr, std::size_t size);

} // end namespace uh::cluster


#endif //CORE_COMMON_H
