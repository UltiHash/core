//
// Created by masi on 13.03.23.
//

#ifndef CORE_SERIALIZATION_H
#define CORE_SERIALIZATION_H

#include "buffered_serializer.h"
#include "deserializer.h"

namespace uh::serialization {

    template <typename Serializer = sl_serializer, typename Deserializer = sl_deserializer>
    requires (is_serializer <Serializer>::value and  is_deserializer <Deserializer>::value)
    class serialization : public Serializer, public Deserializer {
        std::size_t gseek = 0, pseek = 0;
    public:
        explicit serialization (io::device &dev):  Serializer (dev), Deserializer (dev) {}



    };










    class buffered_serialization: public serialization <buffered_serializer <sl_serializer>, sl_deserializer> {
    public:
        explicit buffered_serialization (io::device &dev):  serialization (dev) {}

        template <typename T>
        T read () {
            sync();
            return sl_deserializer::template read <T> ();
        }
    };


} // namespace uh::serialization

#endif //CORE_SERIALIZATION_H
