//
// Created by benjamin-elias on 15.05.23.
//

#include "io/fragmented_device.h"
#include "io/device.h"
#include "serialization/fragment_size_struct.h"
#include "serialization/fragment_serialization.h"

#include <cstdint>
#include <span>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{

    class fragment_on_device : public serialization::fragment_serialization<>, public fragmented_device{

    public:
        /**
         * a fragment_on_device uses a device to either only read or only write in its lifetime
         * after reading or writing the fragment_on_device can tell where to find its content
         * relatively to the device stream.
         * A fragment_on_device is a serialized device.
         *
         * @param dev input device
         * @param index index of fragment needs to be set in case fragment is supposed to be written
         */
        explicit fragment_on_device(io::device& dev,uint8_t index = 0);

        /**
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @return struct with header size written, number of bytes that were persisted to device
         * and the index of the fragment
         */

        uh::serialization::fragment_serialize_size_format write(std::span<const char> buffer) override;

        /**
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @param alloc allocate space on fragment, append multiple times to fragment or limit write
         * @return struct with header size written, number of bytes that were persisted to device
         * and the index of the fragment
         */

        uh::serialization::fragment_serialize_size_format write(std::span<const char> buffer,
                                                                uint32_t alloc) override;

        /**
         * read serialized device to un-serialized buffer
         *
         * @param buffer to be read to from device
         * @return number of bytes totally read from device
         */
        uh::serialization::fragment_serialize_size_format read(std::span<char> buffer) override;

        /**
         *
         * @return the state of the underlying device
         */
        [[nodiscard]] bool valid() const override;

        /**
         * with this function the underlying device is read until
         * one position behind the fragment_on_device content
         *
         * @return counts the entire count a fragment_on_device fills,
         * together with it's header structure
         */
        uh::serialization::fragment_serialize_size_format skip() override;

        /**
         * reset the state machine to a new fragment beginning
         * @return if the underlying device is still valid to deliver the next fragment structure
         */
        bool reset() override;

        /**
         *
         * @return valid index after at least reading once or writing once
         */
        [[nodiscard]] uint8_t getIndex() const;

    private:
        enum{
            UNDEFINED_STATE,
            READING_BEGIN,
            WRITING_BEGIN,
            COMPLETE
        } state_machine = UNDEFINED_STATE;
        int64_t elements_left_to_process{};
        uint8_t index;
        char control_byte{};
    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
