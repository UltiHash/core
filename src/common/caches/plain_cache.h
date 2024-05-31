
#ifndef UH_CLUSTER_PLAIN_CACHE_H
#define UH_CLUSTER_PLAIN_CACHE_H

#include "common/types/scoped_buffer.h"
#include <functional>
#include <zpp_bits.h>

namespace uh::cluster {

class plain_cache {
    unique_buffer<char> m_memory;
    size_t m_pointer = 0;
    const size_t m_entry_size;
    std::function<void(std::span<char>)> m_flush_callback;

public:
    explicit plain_cache(size_t capacity, size_t max_entry_size,
                         std::function<void(std::span<char>)> flush_callback)
        : m_memory(capacity),
          m_entry_size{max_entry_size},
          m_flush_callback{std::move(flush_callback)} {}

    inline void put(const auto& obj) {
        if (m_pointer + m_entry_size > m_memory.size()) {
            flush();
        }
        zpp::bits::out{m_memory.span().subspan(m_pointer),
                       zpp::bits::size4b{}}(obj)
            .or_throw();
        m_pointer += m_entry_size;
    }

    inline void flush() {
        m_flush_callback(m_memory.span().subspan(0, m_pointer));
        m_pointer = 0;
    }

    ~plain_cache() { flush(); }
};

} // namespace uh::cluster
#endif // UH_CLUSTER_PLAIN_CACHE_H
