//
// Created by benjamin-elias on 25.05.23.
//

#ifndef CORE_FRAGMENT_SERIALIZATION_H
#define CORE_FRAGMENT_SERIALIZATION_H

#include "fragment_serializer.h"
#include "fragment_deserializer.h"
#include "buffered_fragment_serializer.h"

namespace uh::serialization {

    template <typename Serializer = sl_fragment_serializer, typename Deserializer = sl_fragment_deserializer>
    requires (is_fragment_serializer <Serializer>::value and is_fragment_deserializer <Deserializer>::value)
    class fragment_serialization : public Serializer, public Deserializer {
    public:
        explicit fragment_serialization (io::device &dev):  Serializer (dev), Deserializer (dev) {}
    };

    class buffered_fragment_serialization: public fragment_serialization <buffered_fragment_serializer <sl_fragment_serializer>, sl_fragment_deserializer> {
    public:
        explicit buffered_fragment_serialization (io::device &dev):  fragment_serialization (dev) {}
    };

} // namespace uh::serialization

#endif //CORE_FRAGMENT_SERIALIZATION_H
