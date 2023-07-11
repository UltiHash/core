#ifndef ENDIAN_CONVERT_H
#define ENDIAN_CONVERT_H

#include <bit>
#include <cstring>
#include <utility>
#include <cinttypes>
#include "check_byteswap.h"

namespace uh::serialization
{

constexpr bool is_big_endian = (std::endian::native == std::endian::big);

template<typename T>
constexpr inline T endian_convert(const T value)
{
    if constexpr (sizeof(value) == 1 or is_big_endian)
    {
        return value;
    }
    else
        if constexpr (!std::is_unsigned_v<T>)
        {
            typedef typename std::conditional<sizeof(T) == 4, std::uint32_t, std::uint64_t>::type uintn_t;
            return std::bit_cast<T>(endian_convert(std::bit_cast<uintn_t>(value)));;
        }
        else
            if constexpr (sizeof(value) == 2)
            {
                return bswap_16(value);
            }
            else
                if constexpr (sizeof(value) == 4)
                {
                    return bswap_32(value);
                }
                else
                    if constexpr (sizeof(value) == 8)
                    {
                        return bswap_64(value);
                    }
}
}

#endif //ENDIAN_CONVERT_H