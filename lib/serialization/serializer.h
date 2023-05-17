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

    public:

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

        explicit sl_serializer (io::device &dev): dev_ (dev) {
        }

        // ---------------------------------------------------------------------

        /**
         * serializes the given data into the device.
         *
         * @tparam ValueType a numerical type
         * @param data to be serialized
         *
         * @return total written size without control byte and data size bytes
         */
        template <typename ValueType>
        requires (std::is_arithmetic_v <ValueType> or std::is_enum_v <ValueType>)
        std::streamsize write (ValueType data) {

            constexpr auto data_size = sizeof (ValueType);
            auto buffer = get_header(data_size);
            std::streamsize header_size = buffer.size();
            const auto* data_ptr = reinterpret_cast <const char *> (&data);
            std::copy (data_ptr,
                       data_ptr + sizeof (data),
                       std::back_inserter(buffer));
            return io::write(dev_, buffer) - header_size;
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
        std::streamsize write (const Range &data) {
            const auto data_size = std::ranges::size (data) * sizeof (InnerType);
            const auto header = get_header(data_size);

            std::streamsize accumulate_size{};
            io::write(dev_, header);
            accumulate_size += io::write(dev_, {reinterpret_cast <const char *> (std::ranges::data(data)), data_size});

            return accumulate_size;
        }


    };

} // namespace uh::serialization

#endif //CORE_SERIALIZER_H
