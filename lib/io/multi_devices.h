//
// Created by benjamin-elias on 18.05.23.
//
#include "io/device.h"

#include <cstdint>
#include <memory>

#ifndef CORE_MULTI_DEVICES_H
#define CORE_MULTI_DEVICES_H

namespace uh::io {

    class multi_devices: device {

    public:

        /**
         * Write the contents of the spans and return the number of bytes written for each
         *
         * @throw writing fails for any reason
         */
        virtual std::vector<std::streamsize> write(std::vector<std::span<const char>> buffer) = 0;

        /**
         * Read from the devices and store it in the buffers. Return the number of
         * bytes read for each.
         *
         * @throw error while reading
         */
        virtual std::vector<std::streamsize> read(std::vector<std::span<char>> buffer) = 0;

    };

    // ---------------------------------------------------------------------

/**
 * Read the complete devices into memory and return it in a vector. `chunk_size`
 * defines how many bytes are read at a time.
 */

template<typename Device>
std::vector<std::vector<char>> read_to_buffer(
        const std::vector<Device&>& dev,
        std::streamsize chunk_size = uh::io::IO_DEFAULT_CHUNK_SIZE);

// ---------------------------------------------------------------------

template<typename Device>
std::vector<std::size_t> write_from_buffer(
        const std::vector<Device&>& dev,
        const std::vector<std::span<char>>& buffer);

// ---------------------------------------------------------------------

/**
 * Copy the complete devices to the given std::ostream&. Return number of
 * bytes written for each.
 */

template<typename Device>
std::vector<std::size_t> copy(const std::vector<Device&>& dev, const std::vector<std::ostream>& out);

// ---------------------------------------------------------------------

/**
 * Copy the complete devices `in` to all the devices `out`. Return number of
 * bytes written for each.
 */

template<typename Device1,typename Device2>
std::vector<std::size_t> copy(const std::vector<Device1&>& in, const std::vector<Device2&>& out);

/**
 * Copy the complete devices `in` to the device `out`. Return number of
 * bytes written.
 */

template<typename Device1,typename Device2>
std::vector<std::size_t> copy(const std::vector<Device1&>& in, Device2& out);

// ---------------------------------------------------------------------

/**
 * Write the buffers to the devices. This function guarantees that the complete
 * buffers are written by calling `dev.write()` repeatedly until everything is
 * written.
 *
 * @throws when `dev.write()` throws
 * @return number of bytes written
 */

template<typename Device>
std::vector<std::streamsize> write(const std::vector<Device&>& dev, const std::vector<std::span<const char>>& buffer);

// ---------------------------------------------------------------------

/**
 * Read from the devices to the buffers. This function guarantees that all bytes
 * available on the devices up to `buffer.size()` are read by calling `dev.read()`
 * repeatedly until `buffer` is filled or `dev.read()` signales end of data.
 *
 * @throws when `dev.read()` throws
 * @return number of bytes read
 */

template<typename Device>
std::vector<std::streamsize> read(const std::vector<Device&>& dev, const std::vector<std::span<char>>& buffer);

// ---------------------------------------------------------------------

} // namespace uh::io

#endif //CORE_MULTI_DEVICES_H
