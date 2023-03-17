//
// Created by masi on 02.03.23.
//


#ifndef CORE_SERIALIZER_H
#define CORE_SERIALIZER_H

#include "serialization_common.h"


namespace uh::serialization {

    class sl_serializer {
    protected:

        io::device &dev_;
        constexpr static char control_byte =  is_big_endian << 7;

        // ---------------------------------------------------------------------

        static constexpr inline void set_control_byte_size_length (char &control_byte, const auto size_length) {
            const auto nl_data_size_len = endian_convert (size_length);
            const char size_len_bits = (static_cast <char> (size_length));
            control_byte |= size_len_bits;
        }

        // ---------------------------------------------------------------------

        static inline void set_data_size (char *buffer, auto data_size, auto data_size_len) {
            const auto nl_data_size = endian_convert (data_size);
            std::memcpy (buffer, reinterpret_cast <const char *> (&nl_data_size) + sizeof (nl_data_size) - data_size_len, data_size_len);
        }

        // ---------------------------------------------------------------------


    public:

        // ---------------------------------------------------------------------

        explicit sl_serializer (io::device &dev): dev_ (dev) {
        }

        // ---------------------------------------------------------------------

        /**
         * serializes the given data into the device.
         *
         * @tparam ValueType a numerical type
         * @param data to be serialized
         */
        template <typename ValueType>
        requires (std::is_arithmetic_v <ValueType> or std::is_enum_v <ValueType>)
        void write (ValueType data) {

            constexpr auto data_size = sizeof (ValueType);
            constexpr auto data_size_len = 1;

            char buffer [1 + data_size_len + data_size];   // control byte + size byte (size len = 1) + data size
            buffer [0] = control_byte;

            set_control_byte_size_length (buffer[0], data_size_len);
            set_data_size(buffer + 1, data_size, data_size_len);
            std::memcpy (buffer + data_size_len + 1, &data, data_size);

            dev_.write (buffer);
        }

        // ---------------------------------------------------------------------

        /**
         * serializes the given range of data into the device.
         *
         * @tparam Range the type of the data to be serialized. It should be a contiguous range of arithmetic types
         * @tparam InnerType the arithmetic inner type of the given range type
         * @param data to be serialized
         */
        template <typename Range, typename InnerType = std::ranges::range_value_t<Range>>
        requires std::ranges::contiguous_range <Range>
                and (std::is_arithmetic_v <InnerType> or std::is_enum_v <InnerType>)
        void write (const Range &data) {
            const auto data_size = std::ranges::size (data);
            auto data_size_len = (std::bit_width (data_size) + 7) / 8;
            data_size_len = (data_size_len == 0)? 1:data_size_len;
            char buffer [1 + data_size_len];   // control byte + size byte
            buffer [0] = control_byte;

            set_control_byte_size_length (buffer[0], data_size_len);
            set_data_size(buffer + 1, data_size, data_size_len);

            dev_.write({buffer, 1 + data_size_len});
            dev_.write({reinterpret_cast <const char *> (std::ranges::data(data)), data_size * sizeof (InnerType)});
        }

        // ---------------------------------------------------------------------

    };

} // namespace uh::serialization

#endif //CORE_SERIALIZER_H
