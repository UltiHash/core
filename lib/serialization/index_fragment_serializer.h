//
// Created by benjamin-elias on 10.07.23.
//

#ifndef INDEX_FRAGMENT_SERIALIZER_H
#define INDEX_FRAGMENT_SERIALIZER_H

#include <io/device.h>

#include <cstring>
#include <algorithm>
#include <array>

#include <serialization/index_fragment_serialization_common.h>

namespace uh::serialization
{

class index_fragment_serializer
{
protected:
    io::device& dev_;

public:
    explicit index_fragment_serializer(io::device& output_device)
        : dev_(output_device)
    {}

    template<typename T>
    requires (std::is_arithmetic_v<T> or std::is_enum_v<T>)
    uint16_t bytes_non_zero(T data)
    {
        uint16_t bytes_non_zero = sizeof(T);
        for (uint64_t i = 0; i < sizeof(T); i++)
        {
            if ((data >> 8 * (sizeof(T) - 1 - i)) == 0)
                bytes_non_zero--;
            else
                break;
        }
        return bytes_non_zero;
    }

    template<typename T>
    requires (std::is_arithmetic_v<T> or std::is_enum_v<T>)
    void write(T data, uint16_t bytes_non_zero = sizeof(T))
    {
        std::vector<char> tmp(bytes_non_zero);
        const auto converted = endian_convert(data);

        std::memcpy(tmp.data(),
                    reinterpret_cast <const char*> (&converted) + sizeof(converted) - bytes_non_zero,
                    bytes_non_zero);

        io::write(dev_, tmp);
    }
};

} // uh::serialization

#endif //INDEX_FRAGMENT_SERIALIZER_H
