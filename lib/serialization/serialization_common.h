//
// Created by masi on 10.03.23.
//

#ifndef CORE_SERIALIZATION_COMMON_H
#define CORE_SERIALIZATION_COMMON_H

#include <bit>
#include <type_traits>
#include <cstring>
#include <utility>
#include <vector>
#include "io/device.h"
#include "endian_convert.h"

namespace uh::serialization {

    template <typename T>
    struct is_serializer: std::bool_constant <
            requires (T t) {
                t.write (std::declval <int> ());
                t.write (std::declval <std::vector <long>> ());
            }>
    {};

    template <typename T>
    struct is_deserializer: std::bool_constant <
            requires (T t) {
                t.template read <int> ();
                t.template read <double> ();
                t.template read <std::vector <int>> ();
            }>
    {};

    template <typename T>
    struct is_serialization_type: std::conjunction <is_serializer <T>, is_deserializer <T>>
    {};

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_COMMON_H
