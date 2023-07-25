//
// Created by benjamin-elias-probst on 7/25/23.
//

#ifndef CORE_TREE_NODE_SERIALIZE_SIZE_FORMAT_H
#define CORE_TREE_NODE_SERIALIZE_SIZE_FORMAT_H

#include <serialization/fragment_serialize_size_format.h>
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

// ---------------------------------------------------------------------

struct tree_node_serialize_size_format
    : public uh::serialization::fragment_serialize_size_format<uint64_t, true>
{

    uint32_t chunk_num_buf_size{};
    uint64_t chunk_num{};

    tree_node_serialize_size_format() = default;

    tree_node_serialize_size_format(uint8_t index_num, uint64_t content_len,
                                    uh::serialization::tree_node_object_type
                                    type = uh::serialization::TREE_NODE,
                                    uint64_t chunk_count = 0)
        : uh::serialization::fragment_serialize_size_format<uint64_t, true>(
        index_num, content_len, type),
          chunk_num(chunk_count)
    {}

    [[nodiscard]] std::vector<char> serialize() override
    {
        uh::io::buffer buf;
        uh::io::write(buf, uh::serialization::fragment_serialize_size_format<
            uint64_t, true>::serialize());
        uh::serialization::index_fragment_serializer ser(buf);

        chunk_num_buf_size = ser.bytes_non_zero(chunk_num);
        char chunk_num_buf_size_serialize[1];
        chunk_num_buf_size_serialize[0] = static_cast<char>(chunk_num_buf_size);
        uh::io::write(buf, chunk_num_buf_size_serialize);

        ser.write(chunk_num, chunk_num_buf_size_serialize[0]);

        return {buf.data().begin(), buf.data().begin() + (long) serialized_size()};
    }

    void deserialize(uh::io::device &input_dev) override
    {
        uh::serialization::fragment_serialize_size_format<
            uint64_t, true>::deserialize(input_dev);
        uh::serialization::index_fragment_deserializer ser(input_dev);

        chunk_num_buf_size = ser.read<unsigned char>();
        chunk_num = ser.read<decltype(chunk_num)>(chunk_num_buf_size);
    }

    [[nodiscard]] inline uint64_t serialized_size() const override
    {
        return uh::serialization::fragment_serialize_size_format<
            uint64_t, true>::serialized_size() +
            1 + chunk_num_buf_size;
    }
};

// ---------------------------------------------------------------------

} // namespace uh::serialization

#endif // CORE_TREE_NODE_SERIALIZE_SIZE_FORMAT_H
