//
// Created by benjamin-elias on 15.05.23.
//

#include <io/device.h>

#include "serialization/serialization.h"

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{
    class fragment : public io::device{

        explicit fragment(device &impl,std::streamoff start_pos = 0);
        std::streamsize write(std::span<const char> buffer) override;
        std::streamsize read(std::span<char> buffer) override;
        [[nodiscard]] bool valid() const override;

    private:
        device &impl;
        bool is_analyzed{};
        std::size_t data_size{};
        std::size_t header_size{};
        std::streamoff frag_beg_pos{};
    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
