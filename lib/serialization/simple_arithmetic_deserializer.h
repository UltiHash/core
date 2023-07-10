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
        std::vector<char> tmp(sizeof(T));
        io::read(dev_, tmp);
        if constexpr (std::endian::native != std::endian::big)
            std::reverse(tmp.begin(), tmp.end());

        T* output_var = reinterpret_cast<T*>(tmp.data());

        return *output_var;
    }
};

} // uh::serialization

#endif //SIMPLE_ARITHMETIC_DESERIALIZER_H
