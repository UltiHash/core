#ifndef CORE_SERIALIZATION_COMMON_H
#define CORE_SERIALIZATION_COMMON_H

#include "endian_convert.h"

#include <type_traits>
#include <vector>
#include "io/device.h"


namespace uh::serialization
{

template<typename T>
struct is_shrink_arithmetic_serializer: std::bool_constant<
    requires(T t) {
        t.write(std::declval<char>());
        t.write(std::declval<short>());
        t.write(std::declval<int>());
        t.write(std::declval<long>());
        t.write(std::declval<unsigned char>());
        t.write(std::declval<unsigned short>());
        t.write(std::declval<unsigned int>());
        t.write(std::declval<unsigned long>());

        t.write(std::declval<char>(), std::declval<char>());
        t.write(std::declval<short>(), std::declval<short>());
        t.write(std::declval<int>(), std::declval<int>());
        t.write(std::declval<long>(), std::declval<long>());
        t.write(std::declval<unsigned char>(), std::declval<unsigned char>());
        t.write(std::declval<unsigned short>(), std::declval<unsigned short>());
        t.write(std::declval<unsigned int>(), std::declval<unsigned int>());
        t.write(std::declval<unsigned long>(), std::declval<unsigned long>());

        t.bytes_non_zero(std::declval<char>());
        t.bytes_non_zero(std::declval<short>());
        t.bytes_non_zero(std::declval<int>());
        t.bytes_non_zero(std::declval<long>());
    }>
{
};

template<typename T>
struct is_shrink_arithmetic_deserializer: std::bool_constant<
    requires(T t) {
        t.template read<char>();
        t.template read<short>();
        t.template read<int>();
        t.template read<long>();
        t.template read<unsigned char>();
        t.template read<unsigned short>();
        t.template read<unsigned int>();
        t.template read<unsigned long>();

        t.template read<char>(std::declval<char>());
        t.template read<short>(std::declval<short>());
        t.template read<int>(std::declval<int>());
        t.template read<long>(std::declval<long>());
        t.template read<char>(std::declval<unsigned char>());
        t.template read<short>(std::declval<unsigned short>());
        t.template read<int>(std::declval<unsigned int>());
        t.template read<long>(std::declval<unsigned long>());
    }>
{
};

template<typename T>
struct is_shrink_arithmetic_serialization_type:
    std::conjunction<is_shrink_arithmetic_serializer<T>, is_shrink_arithmetic_deserializer<T>>
{
};

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_COMMON_H
