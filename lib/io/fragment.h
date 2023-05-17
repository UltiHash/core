//
// Created by benjamin-elias on 15.05.23.
//

#include "io/device.h"

#include "serialization/serialization.h"

#include <openssl/evp.h>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{

    class fragment : public io::device{

    protected:
        io::device &dev_;

    public:
        /**
         * a fragment uses a device to either only read or only write in its lifetime
         * after reading or writing the fragment can tell where to find its content
         * relatively to the device stream.
         * A fragment is a serialized device.
         *
         * @param impl input device
         * @param start_pos relative start position on device to distinguish fragments
         */
        explicit fragment(io::device &dev);

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

        [[nodiscard]] bool valid() const override;

        /**
         * with this function the underlying device is read until
         * one position behind the fragment content
         */
        std::streamsize skip();

    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
