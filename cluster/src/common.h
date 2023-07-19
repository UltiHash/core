//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <forward_list>
#include "big_int.h"

namespace uh::cluster {

//typedef boost::multiprecision::uint128_t uint128_t;

typedef big_int uint128_t;

struct wide_span {
    uint128_t pointer;
    size_t size {};
};

typedef std::forward_list <wide_span> address;

} // end namespace uh::cluster

#endif //CORE_COMMON_H
