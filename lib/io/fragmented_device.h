//
// Created by benjamin-elias on 17.05.23.
//

#include "io/device.h"

#ifndef CORE_FRAGMENTED_DEVICE_H
#define CORE_FRAGMENTED_DEVICE_H

namespace uh::io{

    class fragmented_device : public io::device{

        /**
         * with this function the underlying device is read until
         * one position behind the fragment_device content
         *
         * @return counts the entire count a fragment_device fills,
         * together with it's header structure
         */
         virtual std::streamsize skip() = 0;

    };

}

#endif //CORE_FRAGMENTED_DEVICE_H
