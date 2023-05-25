//
// Created by benjamin-elias on 25.05.23.
//

#ifndef CORE_BUFFERED_FRAGMENT_SERIALIZER_H
#define CORE_BUFFERED_FRAGMENT_SERIALIZER_H

#include <cstring>

#include "fragment_serializer.h"
#include "io/buffered_device.h"

namespace uh::serialization {

    template <typename Serializer = sl_fragment_serializer>
    requires (is_fragment_serializer <Serializer>::value)
    class buffered_fragment_serializer: public Serializer {
        uh::io::buffered_device <io::device> dev_;
    public:
        explicit buffered_fragment_serializer(io::device &dev)
                : Serializer(dev_),
                  dev_(dev, 2^26-1)
        {}

        void sync () {
            dev_.sync();
        }
    };

}

#endif //CORE_BUFFERED_FRAGMENT_SERIALIZER_H
