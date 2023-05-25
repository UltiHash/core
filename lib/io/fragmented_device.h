//
// Created by benjamin-elias on 17.05.23.
//

#include "serialization/fragment_size_struct.h"
#include "span"

#ifndef CORE_FRAGMENTED_DEVICE_H
#define CORE_FRAGMENTED_DEVICE_H

namespace uh::io{

    class fragmented_device{

    public:
        virtual ~fragmented_device() = default;

        /**
         * Write the contents of the span into a non appendable fragment with allocated space from
         * the size of the buffer and return struct that contains header size, content size and
         * index of the fragment that was written
         *
         * @throw writing fails for any reason
         * @return a struct with header size, content size and fragment index
         */
        virtual uh::serialization::fragment_serialize_size_format write(std::span<const char> buffer) = 0;

        /**
         * Write the contents of the span into an appendable fragment with pre-allocated space
         * and return struct that contains header size, content size and index of the fragment that was written
         *
         * @param buffer input buffer
         * @param alloc space that needs to be allocated for the fragment
         * @throw writing fails for any reason
         * @return a struct with header size, content size and fragment index
         */
        virtual uh::serialization::fragment_serialize_size_format write(std::span<const char> buffer,
                                                                        uint32_t alloc) = 0;

        /**
         * Read from the device and store it in the buffer. Return the number of
         * bytes read.
         *
         * @throw error while reading
         * @return a struct with header size, content size and fragment index
         */
        virtual uh::serialization::fragment_serialize_size_format read(std::span<char> buffer) = 0;

        /**
         * Return whether this device still can be used.
         */
        [[nodiscard]] virtual bool valid() const = 0;

        /**
         * with this function the underlying device is read until
         * one position behind the fragment_on_device content
         *
         * @return a struct with header size, content size and fragment index
         */
         virtual uh::serialization::fragment_serialize_size_format skip() = 0;

    };

}

#endif //CORE_FRAGMENTED_DEVICE_H
