//
// Created by benjamin-elias on 18.05.23.
//

#include "fragment_on_device.h"
#include "seekable_device.h"

#ifndef CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H
#define CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H

namespace uh::io {

    class fragment_on_seekable_device: public fragmented_device {

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
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @return number of bytes that were persisted to device
         */

        std::streamsize write(std::span<const char> buffer) override;

        /**
         * read serialized device to un-serialized buffer
         *
         * @param buffer to be read to from device
         * @return number of bytes totally read from device
         */
        std::streamsize read(std::span<char> buffer) override;

        /**
         *
         * @return the state of the underlying device
         */
        [[nodiscard]] bool valid() const override;

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
        fragmented_states state_machine = UNDEFINED_STATE;
        std::streamoff elements_left_to_read{};
    };

} // namespace uh::io

#endif //CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H
