#pragma once

#include "common/types/big_int.h"
#include <cstdint>

namespace uh::cluster {
struct pointer_traits {

    /**
     * TODO: Let's remove this: The storage layer will receive the storage
     * address space pointer, so they do not need to call this function
     * themselves
     *
     * The data store internal pointer is the low number of uint128_t
     *
     * @param global_pointer
     * @return internal data store pointer
     */
    inline static size_t get_pointer(const uint128_t& global_pointer) {
        return global_pointer;
    }

    constexpr static const inline std::size_t group_id_bit_offset = 32 + 64;

    constexpr static const inline std::size_t storage_id_bit_offset = 64;
    /**
     * NOTE: It's for ec group only
     *
     * @param global_pointer
     * @return storage service id
     */
    constexpr inline static uint32_t
    get_service_id(const uint128_t& global_pointer) {
        return global_pointer >> 64;
    }

    /**
     * NOTE: It's for ec group only
     *
     * @param pointer
     * @param storage_id
     * @param data_store_id
     * @return global pointer
     */
    [[nodiscard]] constexpr inline static uint128_t
    get_global_pointer(uint64_t pointer, uint32_t group_id, uint32_t storage_id,
                       uint32_t data_store_id) {
        return (static_cast<uint128_t>(group_id) << group_id_bit_offset) |
               (static_cast<uint128_t>(storage_id) << storage_id_bit_offset) |
               pointer;
    }

    /**
     * @param global_pointer
     * @return group id
     */
    [[nodiscard]] constexpr inline static uint32_t
    get_group_id(uint128_t global_pointer) {
        return global_pointer >> group_id_bit_offset;
    }

    [[nodiscard]] constexpr inline static uint128_t
    get_group_address(uint128_t global_pointer) noexcept {
        return global_pointer & ((uint128_t(1) << group_id_bit_offset) - 1);
    }

    [[nodiscard]] constexpr inline static uint128_t
    set_group_id(uint128_t global_pointer, std::size_t group_id) noexcept {
        return (static_cast<uint128_t>(group_id) << group_id_bit_offset) |
               get_group_address(global_pointer);
    }
};
} // namespace uh::cluster
