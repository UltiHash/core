//
// Created by masi on 10.03.23.
//

#ifndef CORE_SERIALIZATION_COMMON_H
#define CORE_SERIALIZATION_COMMON_H

#include <bit>
#include <type_traits>
#include <cstring>
#include <utility>
#include <vector>
#include <cstdint>
#include "io/device.h"
#include "check_byteswap.h"

namespace uh::serialization {

    constexpr bool is_big_endian = (std::endian::native == std::endian::big);

    template <typename T>
    constexpr inline T endian_convert(const T value) {
        if constexpr (sizeof(value) == 1 or is_big_endian) {
            return value;
        } else if constexpr (!std::is_unsigned_v<T>) {
            typedef typename std::conditional<sizeof(T) == 4, std::uint32_t, std::uint64_t>::type uintn_t;
            return std::bit_cast<T> (endian_convert (std::bit_cast<uintn_t> (value)));;
        } else if constexpr (sizeof(value) == 2) {
            return bswap_16 (value);
        } else if constexpr (sizeof(value) == 4) {
            return bswap_32 (value);
        } else if constexpr (sizeof(value) == 8) {
            return bswap_64 (value);
        }
    }

    template <typename T>
    struct is_serializer: std::bool_constant <
            requires (T t) {
                t.write (std::declval <int> ());
                t.write (std::declval <std::vector <long>> ());
            }>
    {};

    template <typename T>
    struct is_deserializer: std::bool_constant <
            requires (T t) {
                t.template read <int> ();
                t.template read <double> ();
                t.template read <std::vector <int>> ();
            }>
    {};

    template <typename T>
    struct is_serialization_type: std::conjunction <is_serializer <T>, is_deserializer <T>>
    {};



} // namespace uh::serialization

#endif //CORE_SERIALIZATION_COMMON_H
