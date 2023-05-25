//
// Created by benjamin-elias on 17.05.23.
//

#include "serialization/fragment_size_struct.h"
#include "io/device.h"

#include <span>
#include <vector>
#include <ios>

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
         * Read from the fragmented device and store it in the buffer. Return the number of
         * bytes read.
         *
         * @throw error while reading
         * @return a struct with header size, content size and fragment index
         */
        virtual uh::serialization::fragment_serialize_size_format read(std::span<char> buffer) = 0;

        /**
         * Return whether this fragmented device still can be used.
         */
        [[nodiscard]] virtual bool valid() const = 0;

        /**
         * with this function the underlying fragmented device is read until
         * one position behind the fragment_on_device content
         *
         * @return a struct with header size, content size and fragment index
         */
         virtual uh::serialization::fragment_serialize_size_format skip() = 0;

    };

// ---------------------------------------------------------------------

/**
 * Copy the complete fragmented device `in` to the fragmented device `out`. Return number of
 * bytes written.
 */
    std::size_t copy(device& in, fragmented_device& out);

// ---------------------------------------------------------------------

/**
 * Copy the complete fragmented device `in` to the fragmented device `out`. Return number of
 * bytes written.
 */
    std::size_t copy(fragmented_device& in, device& out);

// ---------------------------------------------------------------------

/**
 * Copy the complete fragmented device `in` to the fragmented device `out`. Return number of
 * bytes written.
 */
    std::size_t copy(fragmented_device& in, fragmented_device& out);

// ---------------------------------------------------------------------

/**
 * Write the buffer to the fragmented device. This function guarantees that the complete
 * buffer is written by calling `dev.write()` repeatedly until everything is
 * written.
 *
 * @throws when `dev.write()` throws
 * @return number of bytes written
 */
    serialization::fragment_serialize_size_format write
            (fragmented_device& dev, std::span<const char> buffer);

// ---------------------------------------------------------------------

/**
 * Write the buffer to the fragmented device. This function guarantees that the complete
 * buffer is written by calling `dev.write()` repeatedly until everything is
 * written.
 *
 * @throws when `dev.write()` throws
 * @param alloc reserve space to be written
 * @return number of bytes written
 */
    serialization::fragment_serialize_size_format write
        (fragmented_device& dev, std::span<const char> buffer, uint32_t alloc);

// ---------------------------------------------------------------------

/**
 * Read from the fragmented device to the buffer. This function guarantees that all bytes
 * available on the fragmented device up to `buffer.size()` are read by calling `dev.read()`
 * repeatedly until `buffer` is filled or `dev.read()` signales end of data.
 *
 * @throws when `dev.read()` throws
 * @return number of bytes read
 */
    serialization::fragment_serialize_size_format read
        (fragmented_device& dev, std::span<char> buffer);

// ---------------------------------------------------------------------

}

#endif //CORE_FRAGMENTED_DEVICE_H
