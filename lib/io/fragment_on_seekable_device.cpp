//
// Created by benjamin-elias on 26.05.23.
//

#include "fragment_on_seekable_device.h"

namespace uh::io {

// ---------------------------------------------------------------------

fragment_on_seekable_device::fragment_on_seekable_device(io::seekable_device &dev, uint8_t index):
dev2_(dev), fragment_on_device(dev,index) {}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format fragment_on_seekable_device::skip()
{
    serialization::fragment_serialize_transit_format header_read_format =
            serialization::sl_fragment_deserializer::get_header_data_size_index();

    dev2_.seek(header_read_format.content_size,std::ios_base::cur);

    return {
            static_cast<uint8_t>(header_read_format.header_size),
            header_read_format.content_size,
            header_read_format.index
            };
}

// ---------------------------------------------------------------------

} // namespace uh::io