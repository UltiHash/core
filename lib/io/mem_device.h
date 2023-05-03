//
// Created by masi on 5/2/23.
//

#ifndef CORE_MEM_DEVICE_H
#define CORE_MEM_DEVICE_H

#include "device.h"
#include <vector>

namespace uh::io {

class mem_device : public device {
public:
    mem_device (device& base, std::size_t mem_size);

    std::streamsize write(std::span<const char> buffer) override;

    std::streamsize read(std::span<char> buffer) override;

    [[nodiscard]] bool valid() const override;

private:
    device& m_dev;
    std::vector <char> m_mem;
    std::size_t m_size;
    std::size_t m_index;
};

}

#endif //CORE_MEM_DEVICE_H
