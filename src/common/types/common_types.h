#ifndef CORE_COMMON_TYPES_H
#define CORE_COMMON_TYPES_H

#include "address.h"
#include "big_int.h"
#include "unique_buffer.h"

#include <span>
#include <vector>
#include <zpp_bits.h>

namespace uh::cluster {

static constexpr std::size_t KIBI_BYTE = 1024;
static constexpr std::size_t MEBI_BYTE = 1024 * KIBI_BYTE;
static constexpr std::size_t GIBI_BYTE = 1024 * MEBI_BYTE;
static constexpr std::size_t TEBI_BYTE = 1024 * GIBI_BYTE;
static constexpr std::size_t PEBI_BYTE = 1024 * TEBI_BYTE;

struct dedupe_response {
    std::size_t effective_size{};
    address addr;

    void append(const dedupe_response& other) {
        effective_size += other.effective_size;
        addr.append_address(other.addr);
    }
};

struct directory_message {
    std::string bucket_id;
    zpp::bits::optional_ptr<std::string> object_key;
    zpp::bits::optional_ptr<std::string> object_key_prefix;
    zpp::bits::optional_ptr<std::string> object_key_lower_bound;
    zpp::bits::optional_ptr<address> addr;

    bool operator==(const directory_message& rhs) const {
        bool result = bucket_id == rhs.bucket_id;

        if (object_key.get() != nullptr && rhs.object_key.get() != nullptr)
            result &= *object_key == *rhs.object_key;
        else if (object_key.get() == nullptr && rhs.object_key.get() == nullptr)
            result &= true;
        else
            return false;

        if (object_key_prefix.get() != nullptr &&
            rhs.object_key_prefix.get() != nullptr)
            result &= *object_key_prefix == *rhs.object_key_prefix;
        else if (object_key_prefix.get() == nullptr &&
                 rhs.object_key_prefix.get() == nullptr)
            result &= true;
        else
            return false;

        if (object_key_lower_bound.get() != nullptr &&
            rhs.object_key_lower_bound.get() != nullptr)
            result &= *object_key_lower_bound == *rhs.object_key_lower_bound;
        else if (object_key_lower_bound.get() == nullptr &&
                 rhs.object_key_lower_bound.get() == nullptr)
            result &= true;
        else
            return false;

        if (addr.get() != nullptr && rhs.addr.get() != nullptr)
            result &= *addr == *rhs.addr;
        else if (addr.get() == nullptr && rhs.addr.get() == nullptr)
            result &= true;
        else
            return false;

        return result;
    };
};

struct directory_list_buckets_message {
    std::vector<std::string> entities;
};

struct object {
    std::string name;
    std::string last_modified;
    std::size_t size{};
};

struct directory_list_objects_message {
    std::vector<object> objects;
};

} // end namespace uh::cluster

#endif // CORE_COMMON_TYPES_H
