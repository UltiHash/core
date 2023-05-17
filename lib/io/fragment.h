//
// Created by benjamin-elias on 15.05.23.
//

#include "io/fragmented_device.h"

#include "serialization/serialization.h"

#include <openssl/evp.h>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{

    template<typename DeviceType> requires std::is_base_of_v<io::device, DeviceType>
    class fragment : public io::fragmented_device{

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

        /**
         *
         * @return the state of the underlying device
         */
        [[nodiscard]] bool valid() const override{
            return dev_.valid();
        }

        /**
         * with this function the underlying device is read until
         * one position behind the fragment content
         *
         * @return counts the entire count a fragment fills,
         * together with it's header structure
         */
        std::streamsize skip(){
            auto ser = serialization::serialization(dev_);
            std::span<char>tmp{};

            auto data_size = ser.get_data_size();

            std::streamsize accumulate_read{};
            accumulate_read += std::get<0>(data_size);
            accumulate_read += io::read(dev_, {tmp.data (), std::get<1>(data_size)});

            return accumulate_read;
        }

    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
