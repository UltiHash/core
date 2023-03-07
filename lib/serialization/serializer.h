//
// Created by masi on 02.03.23.
//

#ifndef CORE_SERIALIZER_H
#define CORE_SERIALIZER_H

#include <netinet/in.h>
#include "io/device.h"


namespace uh::serialization {

    class serializer {
    protected:

        io::device &dev_;
        static constexpr bool is_big_endian = (std::endian::native == std::endian::big);
        constexpr static char control_byte =  is_big_endian << 7;

        // ---------------------------------------------------------------------


        static inline auto sized_hton (const auto value) {
            if constexpr (sizeof (value) == 1) {
                return value;
            }
            else if constexpr (sizeof (value) == 2) {
                return htons(value);
            }
            else if constexpr (sizeof (value) <= 4) {
                return htonl(value);
            }
            else {
                auto *val = reinterpret_cast <const unsigned long *> (&value);

                return reinterpret_cast <decltype (value)> (((uint64_t)htonl(*val & 0xFFFFFFFFLL) << 32) | htonl(*val >> 32));
            }
        }

        // ---------------------------------------------------------------------

        static constexpr inline void set_size_length (char &control_byte, const auto size_length) {
            const auto nl_data_size_len = sized_hton (size_length);
            const auto least_significant_byte = nl_data_size_len >> (sizeof(nl_data_size_len) * 8 - 8);
            const char size_len_bits = (least_significant_byte & 0x7);
            control_byte |= size_len_bits;
        }

        // ---------------------------------------------------------------------

        static inline void set_nl_size (char *buffer, auto data_size, auto data_size_len) {
            const auto nl_data_size = sized_hton (data_size);
            std::memcpy (buffer, reinterpret_cast <const char *> (&nl_data_size) + sizeof (nl_data_size) - data_size_len, data_size_len);
        }

        // ---------------------------------------------------------------------


    public:

        // ---------------------------------------------------------------------

        explicit serializer (io::device &dev): dev_ (dev) {
        }

        // ---------------------------------------------------------------------

        /**
         * serializes the given data into the device.
         *
         * @tparam Arithmetic a numerical type
         * @param data to be serialized
         */
        template <typename Arithmetic>
        requires std::is_arithmetic_v <Arithmetic>
        void write (Arithmetic data) {

            constexpr auto data_size = sizeof (Arithmetic);
            constexpr auto data_size_len = 1;

            //const auto nl_data = sized_hton (data);
            //const auto nl_data_size = sized_hton (data_size);

            char buffer [1 + data_size_len + data_size];   // control byte + size byte (size len = 1) + data size
            buffer [0] = control_byte;

            set_size_length (buffer[0], data_size_len);
            set_nl_size (buffer + 1, data_size, data_size_len);
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
                and (std::is_arithmetic_v <InnerType>)
        void write (const Range &data) {
            const auto data_size = std::ranges::size (data);
            //const auto data_size_len = fetch_actual_size_len (data_size);
            auto data_size_len = (std::bit_width (data_size) + 7) / 8;
            data_size_len = (data_size_len == 0)? 1:data_size_len;
            char buffer [1 + data_size_len];   // control byte + size byte
            buffer [0] = control_byte;

            set_size_length (buffer[0], data_size_len);
            set_nl_size (buffer + 1, data_size, data_size_len);

            dev_.write({buffer, 1 + data_size_len});
            dev_.write({reinterpret_cast <const char *> (std::ranges::data(data)), data_size * sizeof (InnerType)});
        }

        // ---------------------------------------------------------------------

    };

} // namespace uh::serialization

#endif //CORE_SERIALIZER_H
