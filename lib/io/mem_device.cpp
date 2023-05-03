//
// Created by masi on 5/2/23.
//

#include "mem_device.h"
#include "util/exception.h"

uh::io::mem_device::mem_device(device &base, std::size_t mem_size) :
m_dev (base),
m_mem (mem_size),
m_size (m_dev.read(m_mem)),
m_index (0)
{}

std::streamsize uh::io::mem_device::write(std::span<const char> buffer) {
    THROW (util::exception, "not implemented");
}

std::streamsize uh::io::mem_device::read(std::span<char> buffer) {

}

bool uh::io::mem_device::valid() const {
    return m_size > 0 && m_dev.valid();
}
