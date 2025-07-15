#pragma once

#include <common/types/address.h>
#include <common/types/big_int.h>
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
    constexpr static const inline std::size_t group_id_bit_offset = 32 + 64;

    struct rr {
        constexpr static const inline std::size_t storage_id_bit_offset = 64;

        inline static std::pair<std::size_t, storage_pointer>
        get_storage_pointer(pointer global_pointer) {
            std::size_t storage_id = (global_pointer >> 64) & 0xFFFFFFFF;
            std::size_t storage_ptr = static_cast<std::size_t>(global_pointer);
            return {storage_id, storage_ptr};
        }

        /**
         * @param pointer
         * @param storage_id
         * @return global pointer
         */
        [[nodiscard]] constexpr inline static pointer
        get_global_pointer(storage_pointer storage_pointer,
                           std::size_t group_id, std::size_t storage_id) {
            return (static_cast<pointer>(group_id) << group_id_bit_offset) |
                   (static_cast<pointer>(storage_id) << storage_id_bit_offset) |
                   storage_pointer;
        }
    };

    struct ec {
        [[nodiscard]] constexpr inline static pointer
        get_global_pointer(storage_pointer storage_pointer, size_t group_id,
                           size_t storage_id, std::size_t chunk_size,
                           std::size_t stripe_size) {
            pointer group_ptr =
                ((pointer)(storage_pointer / chunk_size) * stripe_size) +
                (storage_pointer % chunk_size) +
                ((pointer)chunk_size * storage_id);
            return (static_cast<pointer>(group_id) << group_id_bit_offset) |
                   group_ptr;
        }

        [[nodiscard]] constexpr inline static std::pair<std::size_t,
                                                        storage_pointer>
        get_storage_pointer(pointer group_pointer, std::size_t chunk_size,
                            std::size_t stripe_size) {
            std::size_t group_mod = group_pointer % stripe_size;
            std::size_t storage_id = group_mod / chunk_size;
            std::size_t storage_ptr =
                (group_pointer / stripe_size) * chunk_size +
                (group_mod - storage_id * chunk_size);
            return {storage_id, storage_ptr};
        }
    };

    /**
     * @param global_pointer
     * @return group id
     */
    [[nodiscard]] constexpr inline static std::size_t
    get_group_id(pointer global_pointer) {
        return global_pointer >> group_id_bit_offset;
    }

    [[nodiscard]] constexpr inline static pointer
    get_group_pointer(pointer global_pointer) noexcept {
        return global_pointer & ((pointer(1) << group_id_bit_offset) - 1);
    }
};

} // namespace uh::cluster
