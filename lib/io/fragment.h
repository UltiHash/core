//
// Created by benjamin-elias on 15.05.23.
//

#include <io/device.h>

#include "serialization/serialization.h"

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{
    class fragment : public io::device{

    public:
        /**
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @return number of bytes that were persisted to device
         */
        std::streamsize write(std::span<const char> buffer) override;
        /**
         * read serialized device to unserialized buffer
         *
         * @param buffer to be read to from device
         * @return number of bytes totally read from device
         */
        std::streamsize read(std::span<char> buffer) override;
        [[nodiscard]] bool valid() const override;

        /**
         *
         * @return size of content only
         */
        std::size_t content_size();

        /**
         *
         * @return size of entire fragment with control and size bytes
         */
        std::size_t total_size();

        /**
         *
         * @return stored relative starting position on device
         */
        std::streamoff begin_pos();

        /**
         * this function tries to optimize speed by enabling the developer to allocate a large buffer
         * by knowing how big a buffer needs to be to read the rest of the fragment in a single attempt
         *
         * @return size of leftover fragment size
         */
        std::streamoff ready_to_read();

        /**
         * with this function all the content is read from the device blind to increase its stream
         * pointer to one byte after the fragment
         */
        void skip();
        /**
         * a fragment uses a device to either only read or only write in its lifetime
         * after reading or writing the fragment can tell where to find its content
         * relatively to the device stream
         *
         * @param impl input device
         * @param start_pos relative start position on device to distinguish fragments
         */
        explicit fragment(device &impl,std::streamoff start_pos = 0);

    private:
        device &impl;
        bool is_analyzed{};
        std::size_t data_size{};
        std::size_t header_size{};
        std::streamoff frag_beg_pos{};
        std::streamoff frag_read_pos{};
    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
