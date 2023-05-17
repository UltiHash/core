//
// Created by benjamin-elias on 15.05.23.
//

#include "io/device.h"

#include "serialization/serialization.h"

#include <openssl/evp.h>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{

    template<typename DeviceType> requires std::is_base_of_v<io::device, DeviceType>
    class fragment : public io::device{

    protected:
        DeviceType& dev_;

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
        explicit fragment(DeviceType& dev) : dev_(dev){}

        /**
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @return number of bytes that were persisted to device
         */

        std::streamsize write(std::span<const char> buffer) override{
            auto ser = serialization::serialization(dev_);
            return ser.write(buffer);
        }

        /**
         * read serialized device to un-serialized buffer
         *
         * @param buffer to be read to from device
         * @return number of bytes totally read from device
         */
        std::streamsize read(std::span<char> buffer) override{
            auto ser = serialization::serialization(dev_);
            return ser.read(buffer);
        }

        [[nodiscard]] bool valid() const override{
            return dev_.valid();
        }

        /**
         * with this function the underlying device is read until
         * one position behind the fragment content
         */
        std::streamsize skip(){
            auto ser = serialization::serialization(dev_);
            std::span<char>tmp{};
            return ser.read(tmp);
        }

    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
