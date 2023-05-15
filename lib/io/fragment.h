//
// Created by benjamin-elias on 15.05.23.
//

#include <io/device.h>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{
    class fragment : public io::device{

        explicit fragment(device &impl);
        std::streamsize write(std::span<const char> buffer) override;
        std::streamsize read(std::span<char> buffer) override;
        bool valid() const override;

    private:
        device &impl;
    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
