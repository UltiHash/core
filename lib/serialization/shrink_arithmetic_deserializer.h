#ifndef SHRINK_ARITHMETIC_DESERIALIZER_H
#define SHRINK_ARITHMETIC_DESERIALIZER_H

#include <io/device.h>
#include <serialization/shrink_arithmetic_serialization_common.h>

#include <cstring>
#include <algorithm>

#include <cstdint>

namespace uh::serialization
{

class shrink_arithmetic_deserializer
{
protected:
    io::device& dev_;

public:
    explicit shrink_arithmetic_deserializer(io::device& input_device)
    : dev_(input_device)
        {}

    template<typename T>
    requires (std::is_arithmetic_v<T> or std::is_enum_v<T>)
    T read(uint16_t bytes_non_zero = sizeof(T))
    {
        std::vector<char> tmp(bytes_non_zero);
        io::read(dev_, tmp);
        T sum_result{};

        for(std::size_t i = 0; i < bytes_non_zero; i++){
            T shift_tmp = (unsigned char) tmp[i];
            sum_result += shift_tmp << (8 * (sizeof(T) - 1 - i));
        }

        return endian_convert(sum_result);
    }
};

} // uh::serialization

#endif //SHRINK_ARITHMETIC_DESERIALIZER_H
