//
// Created by masi on 3/29/23.
//
#include "serialization_common.h"

namespace uh::serialization {

    std::streamsize sync_write(io::device &dev, const std::span<const char> buffer) {
        std::size_t written = 0;
        const auto total = buffer.size();
        while (written < total) {
            written += dev.write({buffer.data() + written, total - written});
        }
        return written;
    }

    std::streamsize sync_read(io::device &dev, std::span<char> buffer) {
        std::size_t reads = 0;
        const auto total = buffer.size();
        while (reads < total) {
            reads += dev.read({buffer.data() + reads, total - reads});
        }
        return reads;
    }

}