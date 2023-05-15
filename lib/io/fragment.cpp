//
// Created by benjamin-elias on 15.05.23.
//

#include "fragment.h"

namespace uh::io
{
    fragment::fragment(device &impl) : impl(impl) {
        auto ser = serialization::serialization(impl);
        ser.read<std::vector<char>>();

    }

    std::streamsize fragment::write(std::span<const char> buffer) {
        return 0;
    }

    std::streamsize fragment::read(std::span<char> buffer) {
        return 0;
    }

    bool fragment::valid() const {
        return false;
    }
}


