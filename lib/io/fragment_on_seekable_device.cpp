//
// Created by benjamin-elias on 26.05.23.
//

#include "fragment_on_seekable_device.h"

namespace uh::io
{

// ---------------------------------------------------------------------

fragment_on_seekable_device::fragment_on_seekable_device(io::seekable_device& dev, uint8_t index)
    :
    dev_seek(dev), fragment_on_device(dev, index)
{}

// ---------------------------------------------------------------------

uh::serialization::fragment_serialize_size_format<> fragment_on_seekable_device::skip()
{
    uh::serialization::fragment_serialize_size_format read_over;
    read_over.deserialize(dev_seek);

    dev_seek.seek(read_over.content_size, std::ios_base::cur);

    return read_over;
}

// ---------------------------------------------------------------------

} // namespace uh::io