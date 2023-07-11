#ifndef CORE_FRAGMENT_SIZE_STRUCT_H
#define CORE_FRAGMENT_SIZE_STRUCT_H

#include <serialization/shrink_arithmetic_serializer.h>
#include <serialization/shrink_arithmetic_deserializer.h>
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
    uint16_t index_num{};
    uint16_t content_buf_size{};
    uint32_t content_size{};

    fragment_serialize_size_format() = default;

    fragment_serialize_size_format(uint8_t index_num, uint32_t content_len)
        :
        content_size(content_len), index_num(index_num)
    {}

    [[nodiscard]] std::vector<char> serialize()
    {
        io::buffer buf;
        shrink_arithmetic_serializer ser(buf);
        ser.write((unsigned char) index_num);

        content_buf_size = ser.bytes_non_zero(content_size);
        char content_buf_size_serialize[1];
        content_buf_size_serialize[0] = static_cast<unsigned char>(content_buf_size);
        io::write(buf, content_buf_size_serialize);

        ser.write(content_size, content_buf_size_serialize[0]);

        return {buf.data().begin(), buf.data().begin() + serialized_size()};
    }

    void deserialize(io::device& input_dev)
    {
        shrink_arithmetic_deserializer ser(input_dev);

        index_num = ser.read<unsigned char>();
        content_buf_size = ser.read<unsigned char>();
        content_size = ser.read<decltype(content_size)>(content_buf_size);
    }

    [[nodiscard]] inline long serialized_size() const{
        return 1 + 1 + content_buf_size;
    }
};

// ---------------------------------------------------------------------

}

#endif //CORE_FRAGMENT_SIZE_STRUCT_H
