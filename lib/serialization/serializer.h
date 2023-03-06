//
// Created by masi on 02.03.23.
//

#ifndef CORE_SERIALIZER_H
#define CORE_SERIALIZER_H

#include <netinet/in.h>
#include "io/device.h"

namespace uh::serialization {

    class serialization {

    protected:
        io::device &dev_;

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
                return ((uint64_t)htonl((value) & 0xFFFFFFFFLL) << 32) | htonl((value) >> 32);
            }
        }

        static inline auto sized_ntoh (const auto value) {
            if constexpr (sizeof (value) == 1) {
                return value;
            }
            else if constexpr (sizeof (value) == 2) {
                return ntohs(value);
            }
            else if constexpr (sizeof (value) <= 4) {
                return ntohl(value);
            }
            else {
                return ((uint64_t)ntohl((value) & 0xFFFFFFFFLL) << 32) | ntohl((value) >> 32);
            }
        }

        static constexpr inline void set_size_length (char &control_byte, const auto size_length) {
            const auto nl_data_size_len = sized_hton (size_length);
            const auto least_significant_byte = nl_data_size_len >> (sizeof(nl_data_size_len) * 8 - 8);
            const char size_len_bits = (least_significant_byte & 0x7);
            control_byte |= size_len_bits;
        }


        [[nodiscard]] static inline auto get_size_length (const char control_byte) {
            unsigned long moved_least_significant_byte = 0;
            moved_least_significant_byte |= control_byte & 0x7;
            moved_least_significant_byte = moved_least_significant_byte << (sizeof(moved_least_significant_byte) * 8 - 8);
            unsigned long size_length = sized_ntoh (moved_least_significant_byte);
            return size_length;
        }


        static inline void set_nl_size (char *buffer, auto data_size, auto data_size_len) {
            const auto nl_data_size = sized_hton (data_size);
            std::memcpy (buffer, reinterpret_cast <const char *> (&nl_data_size) + sizeof (nl_data_size) - data_size_len, data_size_len);
        }

        static inline auto get_nl_size (std::vector <char> &buffer, auto data_size_len) {
            //union {
            //    unsigned long value;
            //    char *bytes[sizeof(value)];
            //} nl_data_size {};
            //std::memcpy (&nl_data_size.bytes + sizeof (unsigned long) - data_size_len, buffer.data(), data_size_len);
            //const auto data_size = sized_ntoh (nl_data_size.value);
            unsigned long nl_data_size = 0;
            std::memcpy (reinterpret_cast <char *> (&nl_data_size) + sizeof (unsigned long) - data_size_len, buffer.data(), data_size_len);
            const auto data_size = sized_ntoh (nl_data_size);
            return data_size;
        }

        explicit serialization (io::device &dev): dev_ (dev) {

        }
    };

    class serializer: serialization {
    private:


        static constexpr char control_byte = (std::endian::native == std::endian::big) << 7;



    public:

        explicit serializer (io::device &dev): serialization (dev) {
        }

        template <typename Arithmetic>
        requires std::is_arithmetic_v <Arithmetic>
        void write (Arithmetic data) {

            constexpr auto data_size = sizeof (Arithmetic);
            constexpr auto data_size_len = 1;

            const auto nl_data = sized_hton (data);
            const auto nl_data_size = sized_hton (data_size);

            char buffer [1 + data_size_len + data_size];   // control byte + size byte (size len = 1) + data size
            buffer [0] = 0;

            set_size_length (buffer[0], data_size_len);
            std::memcpy (buffer + 1, &nl_data_size, data_size_len);
            std::memcpy (buffer + data_size_len + 1, &nl_data, data_size);

            dev_.write (buffer);
        }

        template <typename Range, typename InnerType = std::ranges::range_value_t<Range>>
        requires std::ranges::contiguous_range <Range>
                //and (std::is_arithmetic_v <InnerType>)
        void write (const Range &data) {
            const auto data_size = std::ranges::size (data);
            //const auto data_size_len = fetch_actual_size_len (data_size);
            auto data_size_len = (std::bit_width (data_size) + 7) / 8;
            data_size_len = (data_size_len == 0)? 1:data_size_len;

            if constexpr (big_indian or sizeof (InnerType) == 1) {
                char buffer [1 + data_size_len];   // control byte + size byte
                buffer [0] = 0;

                set_size_length (buffer[0], data_size_len);
                set_nl_size (buffer + 1, data_size, data_size_len);

                dev_.write({buffer, 1 + data_size_len});
                dev_.write({std::ranges::data(data), data_size});
            }
            else {
                std::vector <char> buffer;
                buffer.resize (1 + data_size_len + data_size);    // control byte + size byte + data
                buffer [0] = 0;
                set_size_length (buffer[0], data_size_len);
                set_nl_size (buffer.data() + 1, data_size, data_size_len);

                for (long i = 1 + data_size_len; i < buffer.size(); i+=sizeof (InnerType)) {
                    const auto nl_val = sized_hton (data [i - 1 - data_size_len]);
                    std::memcpy (buffer.data() + i, &nl_val, sizeof (nl_val));
                }
                dev_.write (buffer);
            }
        }

    };

    class deserializer: serialization {
        static constexpr unsigned long read_buffer_size = 128;
    public:
        explicit deserializer(io::device &dev) : serialization(dev) {
        }

        template<typename Arithmetic>
        requires std::is_arithmetic_v<Arithmetic>
        Arithmetic read () {

            constexpr auto data_size = sizeof (Arithmetic);
            constexpr auto data_size_len = 1;

            char buffer [1 + data_size_len + data_size];   // control byte + size byte (size len = 1) + data size
            dev_.read (buffer);

            Arithmetic data;
            std::memcpy (&data, buffer + data_size_len + 1, data_size);

            data = sized_ntoh (data);

            return data;
        }

        template <typename Range, typename InnerType = std::ranges::range_value_t<Range>>
        requires std::ranges::contiguous_range <Range>
                 and (big_indian || std::is_arithmetic_v <InnerType>)
                 and requires (Range range, InnerType inner_type) {range.resize (1); range[0] = inner_type;}
        Range read () {
            char control_byte [1];
            dev_.read(control_byte);
            auto data_size_len = get_size_length (control_byte [0]);
            std::vector <char> data_size_bytes (data_size_len);
            dev_.read(data_size_bytes);
            const auto data_size = get_nl_size (data_size_bytes, data_size_len);

            Range range;
            range.resize (data_size);

            if constexpr (big_indian or sizeof (InnerType) == 1) {
                for (long i = 0; i < data_size; i += read_buffer_size) {
                    dev_.read({reinterpret_cast <char *> (std::ranges::data(range)) + i, read_buffer_size});
                }
            }
            else {
                //union {
                //    InnerType value;
                //    char data[sizeof (InnerType)];
                //} buffer;
                char data[sizeof(InnerType)];
                for (long i = 0; i < data_size; i++) {
                    //dev_.read(buffer.data);
                    //range[i] = buffer.value;
                    dev_.read(data);
                    range[i] = reinterpret_cast <InnerType> (sized_ntoh(data));
                }
            }
            return range;
        }
    };

}

#endif //CORE_SERIALIZER_H
