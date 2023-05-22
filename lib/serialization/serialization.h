//
// Created by masi on 13.03.23.
//

#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include "deserializer.h"
#include "serializer.h"
#include "buffered_serializer_2.h"
#include "buffered_serializer.h"
#include "fragment_serializer.h"
#include "fragment_deserializer.h"

namespace uh::serialization {

    template <typename Serializer = sl_serializer, typename Deserializer = sl_deserializer>
    requires (is_serializer <Serializer>::value and is_deserializer <Deserializer>::value)
    class serialization : public Serializer, public Deserializer {
    public:
        explicit serialization (io::device &dev):  Serializer (dev), Deserializer (dev) {}
    };

    template <typename Serializer = sl_fragment_serializer, typename Deserializer = sl_fragment_serializer>
    requires (is_fragment_serializer <Serializer>::value and is_fragment_deserializer <Deserializer>::value)
    class fragment_serialization : public Serializer, public Deserializer {
    public:
        explicit fragment_serialization (io::device &dev):  Serializer (dev), Deserializer (dev) {}
    };

    class buffered_serialization: public serialization <buffered_serializer <sl_serializer>, sl_deserializer> {
    public:
        explicit buffered_serialization (io::device &dev):  serialization (dev) {}
    };

    class buffered_fragment_serialization: public fragment_serialization <buffered_fragment_serialization <sl_fragment_serializer>, sl_fragment_deserializer> {
    public:
        explicit buffered_fragment_serialization (io::device &dev):  fragment_serialization (dev) {}
    };

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H
