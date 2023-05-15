//
// Created by benjamin-elias on 15.05.23.
//

#include "fragment.h"

namespace uh::io
{

// ---------------------------------------------------------------------

fragment::fragment(device &dev, std::streamoff start_pos):impl(dev),frag_beg_pos(start_pos) {}

// ---------------------------------------------------------------------

std::streamsize fragment::write(std::span<const char> buffer) {
    auto ser = serialization::serialization(impl);
    ser.write(std::vector<char>{buffer.begin(),buffer.end()});
}

// ---------------------------------------------------------------------

std::streamsize fragment::read(std::span<char> buffer) {
    return 0;
}

// ---------------------------------------------------------------------

bool fragment::valid() const {
    return false;
}

// ---------------------------------------------------------------------

}


