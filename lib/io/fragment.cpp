//
// Created by benjamin-elias on 15.05.23.
//

#include "fragment.h"

#include "io/buffer.h"

namespace uh::io
{

// ---------------------------------------------------------------------

fragment::fragment(device &dev, std::streamoff start_pos):impl(dev),frag_beg_pos(start_pos),frag_read_pos(start_pos) {}

// ---------------------------------------------------------------------

std::streamsize fragment::write(std::span<const char> buffer) {
    auto header_of_buffer = uh::serialization::serialization
            <uh::serialization::sl_serializer, uh::serialization::sl_deserializer>::get_header(buffer.size());
    header_size = header_of_buffer.size();
    data_size = buffer.size();
    is_analyzed = true;

    auto ser = serialization::serialization(impl);
    return ser.write(buffer);
}

// ---------------------------------------------------------------------

std::streamsize fragment::read(std::span<char> buffer) {
    if(!is_analyzed){
        auto ser = serialization::serialization(impl);
        auto data_size_tup = ser.get_data_size();
        header_size = std::get<0>(data_size_tup);
        data_size = std::get<1>(data_size_tup);
        is_analyzed = true;
        frag_read_pos += header_size;
    }

    auto read_advance = io::read(impl,{buffer.data(),buffer.size()});
    frag_read_pos += read_advance;

    return read_advance;
}

// ---------------------------------------------------------------------

bool fragment::valid() const {
    return impl.valid() && frag_read_pos < frag_beg_pos+header_size+data_size;
}

// ---------------------------------------------------------------------

}


