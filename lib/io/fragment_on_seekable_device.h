//
// Created by benjamin-elias on 18.05.23.
//

#include "fragment_on_device.h"
#include "seekable_device.h"

#ifndef CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H
#define CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H

namespace uh::io {

    class fragment_on_seekable_device: public fragment_on_device {

    public:
        /**
         * a fragment_on_seekable_device uses a device to either only read or only write,
         * or in case seek over the fragment content in its lifetime;
         * after reading or writing the fragment_on_device can tell where to find its content
         * relatively to the device stream.
         * A fragment_on_device is a serialized device.
         *
         * @param impl input device
         * @param start_pos relative start position on device to distinguish fragments
         */
        explicit fragment_on_seekable_device(io::seekable_device& device);

        /**
         * with this function the underlying device is skipped via seeking until
         * one position behind the fragment_on_device content
         *
         * @return counts the entire count a fragment_on_device fills,
         * together with it's header structure
         */
        std::streamsize skip() override;

    private:
        io::seekable_device& dev_;
    };

} // namespace uh::io

#endif //CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H
