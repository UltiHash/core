//
// Created by benjamin-elias on 10.07.23.
//

#ifndef SIMPLE_ARITHMETIC_SERIALIZER_H
#define SIMPLE_ARITHMETIC_SERIALIZER_H

#include <io/device.h>

#include <cstring>
#include <algorithm>
#include <array>

namespace uh::serialization
{

class simple_arithmetic_serializer
{
protected:
    io::device& dev_;

public:
    explicit simple_arithmetic_serializer(io::device& output_device)
        : dev_(output_device)
    {}

    template<typename T>
    requires (std::is_arithmetic_v<T> or std::is_enum_v<T>)
    void write(T data)
    {
        std::array<char, sizeof(T)> tmp;
        for (int i = 0; i < sizeof(T); i++)
            tmp[sizeof(T) - 1 - i] = (data >> (i * 8));

        io::write(dev_, tmp);
    }
};

} // uh::serialization

#endif //SIMPLE_ARITHMETIC_SERIALIZER_H
