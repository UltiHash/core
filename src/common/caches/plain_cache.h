
#ifndef UH_CLUSTER_PLAIN_CACHE_H
#define UH_CLUSTER_PLAIN_CACHE_H

#include "common/types/scoped_buffer.h"
#include <functional>
#include <zpp_bits.h>

namespace uh::cluster {

class plain_cache {
    std::vector<char> m_memory;
    const size_t m_capacity;
    size_t m_elements{0};
    std::function<void(std::span<char>)> m_flush_callback;

public:
    explicit plain_cache(size_t capacity,
                         std::function<void(std::span<char>)> flush_callback)
        : m_capacity(capacity),
          m_flush_callback{std::move(flush_callback)} {}

    inline plain_cache& operator<<(const auto& obj) {
        zpp::bits::out{m_memory, zpp::bits::size4b{}, zpp::bits::append{}}(obj)
            .or_throw();
        m_elements++;
        if (m_elements == m_capacity) {
            flush();
        }
        return *this;
    }

    inline void flush() {
        m_flush_callback(m_memory);
        m_memory.clear();
        m_elements = 0;
    }

    ~plain_cache() { flush(); }
};

} // namespace uh::cluster
#endif // UH_CLUSTER_PLAIN_CACHE_H
