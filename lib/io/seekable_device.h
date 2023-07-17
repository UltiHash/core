//
// Created by benjamin-elias on 12.05.23.
//

#include "device.h"

#include <ios>

#ifndef CORE_SEEKABLE_DEVICE_H
#define CORE_SEEKABLE_DEVICE_H

namespace uh::io {

    class seekable_device: public io::device  {

    public:
        /**
         * seeks stream pointer to position
         *
         * @param off how far we seek
         * @param whence the position we start seeking from
         */
        virtual void seek (std::streamoff off, std::ios_base::seekdir whence) = 0;

    };
} // uh::io

#endif //CORE_SEEKABLE_DEVICE_H
