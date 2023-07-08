//
// Created by benjamin-elias on 22.05.23.
//

#include <cstdint>
#include <ios>

#ifndef CORE_FRAGMENT_SIZE_STRUCT_H
#define CORE_FRAGMENT_SIZE_STRUCT_H

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

    [[nodiscard]] std::ostringstream serialize() const
    {
        std::ostringstream ss;
        ss << content_size;
        ss << index_num;
        ss << header_size;
        return ss;
    }

    void deserialize(std::istringstream& input){
        input.read(reinterpret_cast<char*>(content_size),sizeof(content_size));
        input.read(reinterpret_cast<char*>(header_size),sizeof(header_size));
        input.read(reinterpret_cast<char*>(index_num),sizeof(index_num));
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
