//
// Created by benjamin-elias on 12.05.23.
//

#include "device.h"

#ifndef CORE_SEEKABLE_DEVICE_H
#define CORE_SEEKABLE_DEVICE_H

namespace uh::io {

    class seekable_device: public io::device  {

        virtual void seek (off64_t pos) = 0;

        virtual void seek (off64_t off, int whence) = 0;

        virtual size_t seekable_size() = 0;
    };
} // uh::io

#endif //CORE_SEEKABLE_DEVICE_H
