//
// Created by benjamin-elias on 17.05.23.
//

#include "fragment.h"

namespace uh::io
{

// ---------------------------------------------------------------------

fragment::fragment(device &dev) : dev_(dev){}

// ---------------------------------------------------------------------

std::streamsize fragment::write(std::span<const char> buffer) {
    auto ser = serialization::serialization(dev_);
    return ser.write(buffer);
}

// ---------------------------------------------------------------------

std::streamsize fragment::read(std::span<char> buffer) {
    auto ser = serialization::serialization(dev_);
    return ser.read(buffer);
}

// ---------------------------------------------------------------------

bool fragment::valid() const {
    return dev_.valid();
}

// ---------------------------------------------------------------------

std::streamsize fragment::skip() {
    auto ser = serialization::serialization(dev_);
    std::span<char>tmp{};
    return ser.read(tmp);
}

// ---------------------------------------------------------------------

} // namespace uh::io