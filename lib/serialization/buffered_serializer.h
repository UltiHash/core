//
// Created by masi on 10.03.23.
//


#ifndef CORE_BUFFERED_SERIALIZER_H
#define CORE_BUFFERED_SERIALIZER_H

#include <cstring>

#include "serializer.h"
#include "io/buffered_device.h"

namespace uh::serialization {


    template <typename Serializer = sl_serializer>
    requires (is_serializer <Serializer>::value)
    class buffered_serializer: public Serializer {
        uh::io::buffered_device <io::device> dev_;
    public:
        explicit buffered_serializer(io::device &dev)
            : Serializer(dev_),
              dev_(dev, 1024)
        {}

        void sync () {
            dev_.sync();
        }
    };

}
#endif //CORE_BUFFERED_SERIALIZER_H
