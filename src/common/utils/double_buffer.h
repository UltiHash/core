#ifndef COMMON_UTILS_DOUBLE_BUFFER_H
#define COMMON_UTILS_DOUBLE_BUFFER_H

#include "common/types/scoped_buffer.h"
#include <array>

namespace uh::cluster {

template <typename type> class basic_double_buffer {
public:
    basic_double_buffer(std::size_t size)
        : m_buffers(),
          m_index(0) {
        m_buffers[0].reserve(size);
        m_buffers[1].reserve(size);
    }

    void flip() { m_index = m_index == 0 ? 1 : 0; }

    auto& current() { return m_buffers[m_index]; }

private:
    std::array<std::vector<type>, 2> m_buffers;
    std::size_t m_index;
};

using double_buffer = basic_double_buffer<char>;

} // namespace uh::cluster

#endif
