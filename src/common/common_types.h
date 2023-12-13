//
// Created by masi on 9/6/23.
//

#ifndef CORE_COMMON_TYPES_H
#define CORE_COMMON_TYPES_H

#include <vector>
#include <span>
#include <third-party/zpp_bits/zpp_bits.h>
#include "lib/big_int.h"
#include <lib/unique_buffer.h>
#include <lib/address.h>

namespace uh::cluster {

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

} // end namespace uh::cluster

#endif //CORE_COMMON_TYPES_H
