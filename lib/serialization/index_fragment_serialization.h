#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include "index_fragment_serialization_common.h"
#include "index_fragment_deserializer.h"
#include "index_fragment_serializer.h"

namespace uh::serialization {

    template <typename Serializer = index_fragment_serializer, typename Deserializer = index_fragment_deserializer>
    requires (is_index_fragment_serializer <Serializer>::value and is_index_fragment_deserializer <Deserializer>::value)
    class index_fragment_serialization : public Serializer, public Deserializer {
    public:
        explicit index_fragment_serialization (io::device &dev): Serializer (dev), Deserializer (dev) {}
    };

} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H
