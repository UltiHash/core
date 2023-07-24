#ifndef CORE_FRAGMENT_SIZE_STRUCT_H
#define CORE_FRAGMENT_SIZE_STRUCT_H

#include <serialization/index_fragment_serializer.h>
#include <serialization/index_fragment_deserializer.h>
#include <io/device.h>
#include <io/buffer.h>

#include <cstdint>
#include <ios>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace uh::serialization
{

enum tree_node_object_type: uint8_t
{
    TREE_NODE,
    CHUNK_COLLECTION
};

// ---------------------------------------------------------------------

template<typename CONTENT_SIZE_TYPE = uint32_t, bool is_tree_node_serialize = false>
requires std::is_unsigned_v<CONTENT_SIZE_TYPE>
struct fragment_serialize_size_format
{
    CONTENT_SIZE_TYPE content_size{};
    uint8_t index_num{};

    tree_node_object_type tree_type{};

    uint16_t content_buf_size{};

    fragment_serialize_size_format() = default;

    fragment_serialize_size_format(uint8_t index_num,
                                   CONTENT_SIZE_TYPE content_len,
                                   tree_node_object_type type = TREE_NODE)
        :
        content_size(content_len), index_num(index_num), tree_type(type)
    {}

    [[nodiscard]] std::vector<char> serialize()
    {
        io::buffer buf;
        index_fragment_serializer ser(buf);
        ser.write((unsigned char) index_num);

        content_buf_size = ser.bytes_non_zero(content_size);
        char content_buf_size_serialize[1];
        content_buf_size_serialize[0] = static_cast<unsigned char>(content_buf_size);
        io::write(buf, content_buf_size_serialize);

        ser.write(content_size, content_buf_size_serialize[0]);

        if constexpr (is_tree_node_serialize)
        {
            ser.write((unsigned char) tree_type);
        }

        return {buf.data().begin(), buf.data().begin() + serialized_size()};
    }

    void deserialize(io::device& input_dev)
    {
        index_fragment_deserializer ser(input_dev);

        index_num = ser.read<unsigned char>();
        content_buf_size = ser.read<unsigned char>();
        content_size = ser.read<decltype(content_size)>(content_buf_size);

        if constexpr (is_tree_node_serialize)
        {
            tree_type = static_cast<tree_node_object_type>(ser.read<unsigned char>());
        }
    }

    [[nodiscard]] inline long serialized_size() const
    {
        return 1 + 1 + is_tree_node_serialize + content_buf_size;
    }
};

// ---------------------------------------------------------------------

}

#endif //CORE_FRAGMENT_SIZE_STRUCT_H
