//
// Created by benjamin-elias on 22.05.23.
//

#ifndef CORE_FRAGMENT_SERIALIZER_H
#define CORE_FRAGMENT_SERIALIZER_H

#include "fragment_serialization_common.h"
#include "fragment_size_struct.h"

#include "util/exception.h"


namespace uh::serialization
{

// ---------------------------------------------------------------------

class sl_fragment_serializer
{
protected:

    io::device& dev_;
    constexpr static char control_byte = is_big_endian << 7;

    // ---------------------------------------------------------------------

    static constexpr inline void set_control_byte_size_length(char& control_byte_v, const auto size_length)
    {
        const char size_len_bits = (static_cast <char> (size_length));
        control_byte_v |= size_len_bits;
    }

    // ---------------------------------------------------------------------

    static inline void set_data_size(char* buffer, auto data_size, auto data_size_len)
    {
        const auto nl_data_size = endian_convert(data_size);
        std::memcpy(buffer,
                    reinterpret_cast <const char*> (&nl_data_size) + sizeof(nl_data_size) - data_size_len,
                    data_size_len);
    }

    // ---------------------------------------------------------------------

    [[nodiscard]] static inline std::vector<char> get_header(size_t data_size, uint8_t index)
    {
        auto data_size_len = (std::bit_width(data_size) + 7) / 8;
        std::vector<char> buffer(data_size_len + 2);
        buffer[0] = control_byte;
        set_control_byte_size_length(buffer[0], data_size_len);
        set_data_size(buffer.data() + 1, data_size, data_size_len);
        buffer.back() = static_cast<char>(index);
        return buffer;
    }

    // ---------------------------------------------------------------------

public:

    // ---------------------------------------------------------------------

    explicit sl_fragment_serializer(io::device& dev)
        : dev_(dev)
    {
    }

    // ---------------------------------------------------------------------

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
    fragment_serialize_size_format write(const Range& data, uint8_t index)
    {
        const auto data_size = std::ranges::size(data) * sizeof(InnerType);
        const auto header = get_header(data_size, index);

        return {static_cast<uint8_t>(io::write(dev_, header)),
                static_cast<uint32_t>(io::write(dev_, {reinterpret_cast <const char*>
                                                       (std::ranges::data(data)), data_size})),
                index};
    }

    // ---------------------------------------------------------------------

    /**
     * serializes the given range of data into the device.
     *
     * @tparam Range the type of the data to be serialized. It should be a contiguous range of arithmetic types
     * @tparam InnerType the arithmetic inner type of the given range type
     * @tparam ALLOC_UTYPE the type of incoming allocation u_int to reserve more or limit the space to be written
     * @param data to be serialized
     */
    template<typename Range, typename InnerType = std::ranges::range_value_t<Range>, typename ALLOC_UTYPE>
    requires std::ranges::contiguous_range<Range>
        and (std::is_arithmetic_v<InnerType> or std::is_enum_v<InnerType>)
        and std::is_unsigned_v<ALLOC_UTYPE> and (sizeof(ALLOC_UTYPE) < sizeof(uint64_t))
    fragment_serialize_size_format write(const Range& data, uint8_t index, ALLOC_UTYPE alloc)
    {
        const auto data_size = alloc * sizeof(InnerType);
        const auto header = get_header(data_size, index);

        return {static_cast<uint8_t>(io::write(dev_, header)),
                static_cast<uint32_t>(io::write(dev_, {reinterpret_cast <const char*>
                                                       (std::ranges::data(data)),
                                                       std::min(data_size, std::ranges::size(data))})),
                index};
    }

    // ---------------------------------------------------------------------

};

} // namespace uh::serialization

#endif //CORE_FRAGMENT_SERIALIZER_H
