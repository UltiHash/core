#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include "shrink_arithmetic_serialization_common.h"
#include "shrink_arithmetic_deserializer.h"
#include "shrink_arithmetic_serializer.h"

namespace uh::serialization {

    template <typename Serializer = shrink_arithmetic_serializer, typename Deserializer = shrink_arithmetic_deserializer>
    requires (is_shrink_arithmetic_serializer <Serializer>::value and is_shrink_arithmetic_deserializer <Deserializer>::value)
    class shrink_arithmetic_serialization : public Serializer, public Deserializer {
    public:
        explicit shrink_arithmetic_serialization (io::device &dev): Serializer (dev), Deserializer (dev) {}
    };

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H
