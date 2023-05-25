//
// Created by masi on 13.03.23.
//

#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include "deserializer.h"
#include "serializer.h"
#include "buffered_serializer_2.h"
#include "buffered_serializer.h"

namespace uh::serialization {

    template <typename Serializer = sl_serializer, typename Deserializer = sl_deserializer>
    requires (is_serializer <Serializer>::value and is_deserializer <Deserializer>::value)
    class serialization : public Serializer, public Deserializer {
    public:
        explicit serialization (io::device &dev):  Serializer (dev), Deserializer (dev) {}
    };

    class buffered_serialization: public serialization <buffered_serializer <sl_serializer>, sl_deserializer> {
    public:
        explicit buffered_serialization (io::device &dev):  serialization (dev) {}
    };

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H
