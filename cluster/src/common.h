//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <vector>
#include <span>
#include "big_int.h"

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
    DEDUPE,
    FAILURE
};

void handle_failure (const std::string& job_name, int target, const std::exception &e);

} // end namespace uh::cluster

#endif //CORE_COMMON_H
