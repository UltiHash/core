//
// Created by masi on 06.03.23.
//

#ifndef CORE_DESERIALIZER_H
#define CORE_DESERIALIZER_H

#include "serialization_common.h"

namespace uh::serialization {

    class sl_deserializer {

    protected:
        io::device &dev_;

        constexpr inline static bool is_different_endian(const char control_byte) {
            return static_cast <bool> (control_byte & 0x80) != is_big_endian;
        }

        // ---------------------------------------------------------------------

        [[nodiscard]] static inline auto get_control_byte_size_length(const char control_byte) {
            unsigned long moved_least_significant_byte = 0;
            moved_least_significant_byte |= control_byte & 0x7;
            moved_least_significant_byte =
                    moved_least_significant_byte << (sizeof(moved_least_significant_byte) * 8 - 8);
            unsigned long size_length = endian_convert(moved_least_significant_byte);
            return size_length;
        }

        // ---------------------------------------------------------------------

        static inline auto get_control_byte_size(std::vector<char> &buffer, auto data_size_len) {
            unsigned long nl_data_size = 0;
            std::memcpy(reinterpret_cast <char *> (&nl_data_size) + sizeof(unsigned long) - data_size_len,
                        buffer.data(), data_size_len);
            const auto data_size = endian_convert (nl_data_size);
            return data_size;
        }

        // ---------------------------------------------------------------------

    public:
        explicit sl_deserializer (io::device &dev) : dev_ (dev) {
        }

        // ---------------------------------------------------------------------

        /**
         * reads the data from the device and deserializes it into the given Arithmetic type.
         * @tparam Arithmetic a numerical type
         * @return deserialized data
         */
        template<typename Arithmetic>
        requires std::is_arithmetic_v<Arithmetic>
        Arithmetic read() {

            constexpr auto data_size = sizeof(Arithmetic);
            constexpr auto data_size_len = 1;

            char buffer[1 + data_size_len + data_size];
            if (io::read(dev_, buffer) == 0)
                throw std::runtime_error("Device is empty.");

            Arithmetic data = *reinterpret_cast <Arithmetic *> (buffer + data_size_len + 1);
            //std::memcpy(&data, buffer + data_size_len + 1, data_size);

            if (is_different_endian(buffer[0])) [[unlikely]] {
                data = endian_convert (data);
            }

            return data;
        }

        // ---------------------------------------------------------------------

        /**
         * putting the function to public makes it possible to speed up by seeking
         * over seekable device
         *
         * @return pair of control byte and the number of bytes forming a buffer to describe the number
         * of elements on the serialization
         */
        [[nodiscard]] inline auto get_control_byte_size_length(){
            char control_byte[1];
            io::read(dev_, control_byte);

            return std::make_pair(control_byte[0],get_control_byte_size_length(control_byte[0]));
        }

        /**
         * This function reads from device
         *
         * @return tuple of
         * 1. number of bytes describing the content with the help
         * of one control bytes and data size buffer and
         * 2. number of elements stored of that type
         * 3. control byte
         */
        auto get_data_size(){
            auto data_size_len = get_control_byte_size_length();
            std::vector<char> data_size_bytes(data_size_len.second);
            io::read(dev_, data_size_bytes);

            auto data_size = get_control_byte_size(data_size_bytes, data_size_len.second);
            if (is_different_endian(data_size_len.first)) {
                data_size = endian_convert (data_size);
            }

            return std::make_tuple(sizeof(data_size_len.first)+data_size_bytes.size(),
                                   data_size,
                                   data_size_len.first);
        }

        // ---------------------------------------------------------------------

        /**
         * reads the data from the device and deserializes it into the given contiguous range type.
         * @tparam Range the type of the data to be deserialized. It should be a contiguous range of arithmetic types
         * @tparam InnerType the arithmetic inner type of the given range type
         * @return deserialized data
         */
        template<typename Range, typename InnerType = std::ranges::range_value_t <Range>>
        requires std::ranges::contiguous_range<Range>
                 and (std::is_arithmetic_v < InnerType > )
                 and requires(Range range, InnerType inner_type) { range.resize(1); range[0] = inner_type; }
        Range read() {

            auto data_size = get_data_size();

            Range range;
            range.resize(std::get<1>(data_size) / sizeof (InnerType));

            if (!is_different_endian(std::get<2>(data_size)) or sizeof(InnerType) == 1) [[likely]] {  // no need to convert
                io::read(dev_, {reinterpret_cast <char *> (std::ranges::data(range)), std::get<1>(data_size)});
            }
            else if constexpr (sizeof(InnerType) > 1) {  // different endian
                char data[sizeof(InnerType)];
                for (auto i = 0u; i < std::get<1>(data_size); i++) {
                    io::read(dev_, data);
                    char *tmp_dat = endian_convert (data);
                    auto *val = reinterpret_cast <InnerType *> (tmp_dat);
                    range[i] = *val;
                }
            }

            return range;

        }


        void read(std::span <char> &range) {

            auto data_size = get_data_size();

            io::read(dev_, {range.data (), std::get<1>(data_size)});
            range = {range.data (), std::get<1>(data_size)};

        }

        // ---------------------------------------------------------------------

    };
} // namespace uh::serialization

#endif //CORE_DESERIALIZER_H
