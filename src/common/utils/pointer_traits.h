#pragma once

#include "common/types/big_int.h"
#include <cstdint>

namespace uh::cluster {
struct pointer_traits {

    /**
     * The data store internal pointer is the low number of uint128_t
     * @param global_pointer
     * @return internal data store pointer
     */
    inline static size_t get_pointer(const uint128_t& global_pointer) {
        return global_pointer.get_low();
    }

    /**
     * @param global_pointer
     * @return storage service id
     */
    inline static uint32_t get_service_id(const uint128_t& global_pointer) {
        return global_pointer.get_high() & 0xFFFFFFFF;
    }

    /**
     * @param global_pointer
     * @return group id
     */
    inline static uint32_t get_group_id(const uint128_t& global_pointer) {
        return (global_pointer.get_high() >> 32) & 0xFFFFFFFF;
    }

    /**
     * @param pointer
     * @param storage_id
     * @param data_store_id
     * @return global pointer
     */
    inline static uint128_t get_global_pointer(size_t pointer,
                                               uint32_t group_id,
                                               uint32_t storage_id,
                                               uint32_t data_store_id) {
        uint64_t high_num = ((uint64_t)group_id << 32) | ((uint64_t)storage_id);
        return {high_num, pointer};
    }
};
} // namespace uh::cluster
