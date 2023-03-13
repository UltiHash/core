//
// Created by masi on 13.03.23.
//

#ifndef CORE_BUFFERED_SERIALIZER_2_H
#define CORE_BUFFERED_SERIALIZER_2_H

#include <cstring>

#include "serializer.h"
#include "io/device.h"

namespace uh::serialization {

    class buffered_serializer_2 : private serializer {
    private:

        static constexpr unsigned long buffer_size = 1024;
        char buffer[buffer_size]{};
        unsigned long pointer = 0;

    public:
        explicit buffered_serializer_2 (io::device &dev) : serializer(dev) {
        }

        /**
         * serializes the given data into the device.
         *
         * @tparam ValueType a numerical type
         * @param data to be serialized
         */
        template<typename ValueType>
        requires (std::is_arithmetic_v<ValueType> or std::is_enum_v<ValueType>)
        void write(ValueType data) {

            constexpr auto data_size = sizeof(ValueType);
            constexpr auto data_size_len = 1;
            constexpr auto total_size = 1 + data_size_len + data_size;

            if (pointer + total_size >= buffer_size) {
                sync();
            }

            buffer[pointer] = control_byte;

            set_control_byte_size_length(buffer[pointer], data_size_len);
            set_data_size(buffer + pointer + 1, data_size, data_size_len);
            std::memcpy(buffer + pointer + data_size_len + 1, &data, data_size);

            pointer += total_size;

        }

        /**
         * serializes the given range of data into the device.
         *
         * @tparam Range the type of the data to be serialized. It should be a contiguous range of arithmetic types
         * @tparam InnerType the arithmetic inner type of the given range type
         * @param data to be serialized
         */
        template<typename Range, typename InnerType = std::ranges::range_value_t<Range>>
        requires std::ranges::contiguous_range<Range>
                 and (std::is_arithmetic_v<InnerType> or std::is_enum_v<InnerType>)
        void write(const Range &data) {
            const auto data_size = std::ranges::size(data);
            auto data_size_len = (std::bit_width(data_size) + 7) / 8;
            data_size_len = (data_size_len == 0) ? 1 : data_size_len;
            const auto total_size = 1 + data_size_len + data_size;

            if (pointer + total_size >= buffer_size) {
                sync();
            }
            if (total_size <= buffer_size) {

                buffer[pointer] = control_byte;
                set_control_byte_size_length(buffer[pointer], data_size_len);
                set_data_size(buffer + pointer + 1, data_size, data_size_len);
                std::memcpy(buffer + pointer + data_size_len + 1,
                            reinterpret_cast <const char *> (std::ranges::data(data)), data_size);
                pointer += total_size;
            } else {
                serializer::write(data);
            }
        }

        void sync() {
            dev_.write({buffer, pointer});
            pointer = 0;
            // also maybe a sync on the device?
        }

        ~buffered_serializer_2() {
            sync();
        }
    };
}

#endif //CORE_BUFFERED_SERIALIZER_2_H
