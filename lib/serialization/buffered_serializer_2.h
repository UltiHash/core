//
// Created by masi on 13.03.23.
//

#ifndef CORE_BUFFERED_SERIALIZER_2_H
#define CORE_BUFFERED_SERIALIZER_2_H

#include <cstring>

#include "io/device.h"

namespace uh::serialization {

    class buffered_serializer_2 {
    private:
        constexpr static char control_byte =  is_big_endian << 7;
        static constexpr unsigned long buffer_size = 1024;
        char buffer[buffer_size] {};
        unsigned long pointer = 0;
        io::device &dev_;

        // ---------------------------------------------------------------------

        static constexpr inline void set_control_byte_size_length (char &control_byte_v, const auto size_length) {
            const char size_len_bits = (static_cast <char> (size_length));
            control_byte_v |= size_len_bits;
        }

        // ---------------------------------------------------------------------

        static inline void set_data_size (char *buffer, auto data_size, auto data_size_len) {
            const auto nl_data_size = endian_convert (data_size);
            std::memcpy (buffer, reinterpret_cast <const char *> (&nl_data_size) + sizeof (nl_data_size) - data_size_len, data_size_len);
        }

        // ---------------------------------------------------------------------

        [[nodiscard]] static inline std::vector <char> get_header (size_t data_size) {
            auto data_size_len = (std::bit_width (data_size) + 7) / 8;
            std::vector <char> buffer (data_size_len + 1);
            buffer [0] = control_byte;
            set_control_byte_size_length (buffer[0], data_size_len);
            set_data_size(buffer.data() + 1, data_size, data_size_len);
            return buffer;
        }


        // ---------------------------------------------------------------------
    public:
        explicit buffered_serializer_2 (io::device &dev): dev_ (dev) {
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

            static_assert (total_size < buffer_size);

            if (pointer + total_size >= buffer_size) {
                sync();
            }

            const auto header = get_header (data_size);
            std::memcpy (buffer + pointer, header.data (), header.size ());
            std::memcpy (buffer + pointer + header.size (), &data, data_size);

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
            const auto header = get_header (data_size * sizeof (InnerType));

            const auto total_size = header.size () + data_size * sizeof (InnerType);
            if (pointer + total_size >= buffer_size) {
                sync();
            }
            if (total_size <= buffer_size) {
                std::memcpy (buffer + pointer, header.data (), header.size ());
                std::memcpy(buffer + pointer + header.size (),
                            std::ranges::data(data), data_size * sizeof (InnerType));
                pointer += total_size;
            } else {
                dev_.write(header);
                dev_.write({reinterpret_cast <const char *> (std::ranges::data(data)), data_size * sizeof (InnerType)});
            }
        }

        void sync() {
            dev_.write(std::span <char> {buffer, pointer});
            pointer = 0;
            // also maybe a sync on the device?
        }

        ~buffered_serializer_2() {
            sync();
        }
    };
}

#endif //CORE_BUFFERED_SERIALIZER_2_H
