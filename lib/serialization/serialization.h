//
// Created by masi on 10.03.23.
//

#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include <bit>
#include <byteswap.h>
#include <type_traits>
#include <cstring>
#include "io/device.h"

namespace uh::serialization {

    class serialization {
    protected:
        io::device &dev_;
        static constexpr bool is_big_endian = (std::endian::native == std::endian::big);

        template <typename T>
        constexpr static inline T endian_convert(const T value) {
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

        explicit serialization(io::device &dev) : dev_(dev) {
        }
    };

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H