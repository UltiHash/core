//
// Created by masi on 10.03.23.
//


#ifndef CORE_BUFFERED_SERIALIZER_H
#define CORE_BUFFERED_SERIALIZER_H

#include <cstring>

#include "serializer.h"
#include "io/buffered_device.h"

namespace uh::serialization {


    class buffered_serializer: public serializer {
        uh::io::buffered_device <io::device> dev_;
    public:
        explicit buffered_serializer (io::device &dev): dev_(dev), serializer(dev_){}
    };


}
#endif //CORE_BUFFERED_SERIALIZER_H
