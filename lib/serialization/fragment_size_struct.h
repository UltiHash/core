#ifndef CORE_FRAGMENT_SIZE_STRUCT_H
#define CORE_FRAGMENT_SIZE_STRUCT_H

#include <serialization/simple_arithmetic_serializer.h>
#include <serialization/simple_arithmetic_deserializer.h>
#include <io/device.h>
#include <io/buffer.h>

#include <cstdint>
#include <ios>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace uh::serialization
{

// ---------------------------------------------------------------------

struct fragment_serialize_size_format
{
    uint32_t content_size{};
    uint16_t header_size{};
    uint16_t index_num{};

    fragment_serialize_size_format() = default;

    fragment_serialize_size_format(uint8_t header_len, uint32_t content_len, uint8_t index_num)
        :
        header_size(header_len), content_size(content_len), index_num(index_num)
    {}

    [[nodiscard]] std::vector<char> serialize() const
    {
        io::buffer buf;
        simple_arithmetic_serializer ser(buf);
        ser.write(content_size);
        ser.write(header_size);
        ser.write(index_num);

        std::vector<char> out_buf(sizeof(content_size) + sizeof(header_size) + sizeof(index_num));
        io::read(buf, out_buf);

        return out_buf;
    }

    void deserialize(io::device& input_dev)
    {
        simple_arithmetic_deserializer ser(input_dev);

        content_size = ser.read<decltype(content_size)>();
        header_size = ser.read<decltype(header_size)>();
        index_num = ser.read<decltype(index_num)>();
    }

};

// ---------------------------------------------------------------------

struct fragment_serialize_transit_format
{
    uint32_t content_size;
    uint16_t header_size;
    char control_byte;
    uint8_t index;

    fragment_serialize_transit_format(uint8_t header_len, uint32_t content_len, char control_byte, uint8_t index)
        :
        header_size(header_len), content_size(content_len), control_byte(control_byte), index(index)
    {}

};

// ---------------------------------------------------------------------

struct fragment_serialize_control_byte_transit_format
{
    uint8_t header_size;
    char control_byte;

    fragment_serialize_control_byte_transit_format(uint8_t header_len, char control_byte)
        :
        header_size(header_len), control_byte(control_byte)
    {}

};

// ---------------------------------------------------------------------

}

#endif //CORE_FRAGMENT_SIZE_STRUCT_H
