//
// Created by benjamin-elias on 10.07.23.
//

#ifndef SIMPLE_ARITHMETIC_DESERIALIZER_H
#define SIMPLE_ARITHMETIC_DESERIALIZER_H

#include <io/device.h>

#include <cstring>
#include <algorithm>

#include <cstdint>

namespace uh::serialization
{

class simple_arithmetic_deserializer
{
protected:
    io::device& dev_;

public:
    explicit simple_arithmetic_deserializer(io::device& input_device)
    : dev_(input_device)
        {}

    template<typename T>
    requires (std::is_arithmetic_v<T> or std::is_enum_v<T>)
    T read()
    {
        char tmp[sizeof(T)];
        io::read(dev_, tmp);
        T sum_result{};

        for(std::size_t i = 0; i < sizeof(T); i++){
            T shift_tmp = (unsigned char) tmp[i];
            sum_result += shift_tmp << (8 * (sizeof(T) - 1 - i));
        }

        return sum_result;
    }
};

} // uh::serialization

#endif //SIMPLE_ARITHMETIC_DESERIALIZER_H
