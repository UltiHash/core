
#ifndef UH_CLUSTER_POINTER_TRAITS_H
#define UH_CLUSTER_POINTER_TRAITS_H

#include "common/types/big_int.h"
#include <cstdint>

namespace uh::cluster {
    struct pointer_traits {
        inline static uint32_t get_data_store_id (const uint128_t& global_pointer) {
            return global_pointer.get_high() & 0xFFFFFFFF;
        }

        inline static size_t get_pointer (const uint128_t& global_pointer) {
            return global_pointer.get_low();
        }

        inline static uint32_t get_service_id (const uint128_t& global_pointer) {
            return global_pointer.get_high() >> 32;
        }

        inline static uint128_t get_global_pointer (size_t pointer, uint32_t storage_id, uint32_t data_store_id) {
            uint64_t high_num = storage_id;
            high_num <<= 32;
            high_num |= data_store_id;
            return {high_num, pointer};
        }
    };
}

#endif // UH_CLUSTER_POINTER_TRAITS_H
