//
// Created by masi on 13.03.23.
//

#include "sstream_device.h"

std::streamsize uh::io::sstream_device::write(std::span<const char> buffer) {
    m_io.write(buffer.data(), buffer.size());
    return buffer.size();
}

std::streamsize uh::io::sstream_device::read(std::span<char> buffer) {
    m_io.read(buffer.data(), buffer.size());
    return m_io.gcount();
}

bool uh::io::sstream_device::valid() const {
    return m_io.good();
}
