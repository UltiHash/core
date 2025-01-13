#pragma once

#include "common/types/big_int.h"
#include <cstdint>

namespace uh::cluster {
struct pointer_traits {

    /**
     * The data store id is in the lowest 32 bits of the high number of
     * uint128_t
     * @param global_pointer
     * @return data store id
     */
    inline static uint32_t get_data_store_id(uint128_t global_pointer) {
        return global_pointer.get_high() & 0xFFFFFFFF;
    }

    /**
     * The data store internal pointer is the low number of uint128_t
     * @param global_pointer
     * @return internal data store pointer
     */
    inline static size_t get_pointer(uint128_t global_pointer) {
        return global_pointer.get_low();
    }

    /**
     * The storage service id is the highest 32 bits of uint128_t
     * @param global_pointer
     * @return storage service id
     */
    inline static uint32_t get_service_id(uint128_t global_pointer) {
        return global_pointer.get_high() >> 32;
    }

    /**
     * The global pointer is consisted of:
     *  1. 32 bits for storage id
     *  2. 32 bits for data store id
     *  3. 64 bits for internal data store pointer
     *
     * @param pointer
     * @param storage_id
     * @param data_store_id
     * @return global pointer
     */
    inline static uint128_t get_global_pointer(size_t pointer,
                                               uint32_t storage_id,
                                               uint32_t data_store_id) {
        uint64_t high_num = storage_id;
        high_num <<= 32;
        high_num |= data_store_id;
        return {high_num, pointer};
    }
};
} // namespace uh::cluster
