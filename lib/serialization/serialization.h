//
// Created by masi on 13.03.23.
//

#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include "buffered_serializer.h"
#include "deserializer.h"

namespace uh::serialization {

    class buffered_serialization : public buffered_serializer, public deserializer {
    public:
        explicit buffered_serialization (io::device &dev): buffered_serializer(dev), deserializer (dev) {}
    };
} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H
