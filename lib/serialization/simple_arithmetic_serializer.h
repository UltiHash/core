//
// Created by benjamin-elias on 10.07.23.
//

#ifndef SIMPLE_ARITHMETIC_SERIALIZER_H
#define SIMPLE_ARITHMETIC_SERIALIZER_H

#include <io/device.h>

#include <cstring>
#include <algorithm>

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
        std::vector<char> tmp(sizeof(data));
        tmp.assign(&data, &data + sizeof(T));
        if constexpr (std::endian::native != std::endian::big)
            std::reverse(tmp.begin(), tmp.end());

        io::write(dev_, tmp);
    }
};

} // uh::serialization

#endif //SIMPLE_ARITHMETIC_SERIALIZER_H
