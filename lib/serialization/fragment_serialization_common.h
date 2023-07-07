//
// Created by benjamin-elias on 25.05.23.
//

#ifndef CORE_FRAGMENT_SERIALIZATION_COMMON_H
#define CORE_FRAGMENT_SERIALIZATION_COMMON_H

#include "serialization/serialization_common.h"

#include <bit>
#include <type_traits>
#include <utility>
#include <vector>
#include "check_byteswap.h"

namespace uh::serialization
{

template<typename T>
struct is_fragment_serializer: std::bool_constant<
    requires(T t) {
        t.write(std::declval<std::vector<char>>(), std::declval<uint8_t>());
        t.write(std::declval<std::vector<long>>(), std::declval<uint8_t>());
        t.write(std::declval<std::vector<char>>(), std::declval<uint8_t>(),
                std::declval<uint8_t>());
        t.write(std::declval<std::vector<long>>(), std::declval<uint8_t>(),
                std::declval<uint8_t>());
        t.write(std::declval<std::vector<char>>(), std::declval<uint8_t>(),
                std::declval<uint16_t>());
        t.write(std::declval<std::vector<long>>(), std::declval<uint8_t>(),
                std::declval<uint16_t>());
        t.write(std::declval<std::vector<char>>(), std::declval<uint8_t>(),
                std::declval<uint32_t>());
        t.write(std::declval<std::vector<long>>(), std::declval<uint8_t>(),
                std::declval<uint32_t>());
    }>
{
};

template<typename T>
struct is_fragment_deserializer: std::bool_constant<
    requires(T t) {
        t.template read<std::vector<int>>();
        t.template read<std::vector<char>>();
    }>
{
};

template<typename T>
struct is_fragment_serialization_type: std::conjunction<is_fragment_serializer<T>, is_fragment_deserializer<T>>
{
};

} // namespace uh::serialization


#endif //CORE_FRAGMENT_SERIALIZATION_COMMON_H
