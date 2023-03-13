//
// Created by masi on 13.03.23.
//

#ifndef CORE_SSTREAM_DEVICE_H
#define CORE_SSTREAM_DEVICE_H

#include "device.h"

#include <filesystem>
#include <sstream>

namespace uh::io {

// ---------------------------------------------------------------------

    class sstream_device : public io::device {
        public:

        std::streamsize write(std::span<const char> buffer) override;
        std::streamsize read(std::span<char> buffer) override;
        bool valid() const override;

        private:
        std::stringstream m_io;
    };

} // namespace uh::io


#endif //CORE_SSTREAM_DEVICE_H
