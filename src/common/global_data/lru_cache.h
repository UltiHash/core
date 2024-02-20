#ifndef UH_CLUSTER_LRU_CACHE_H
#define UH_CLUSTER_LRU_CACHE_H

#include <list>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

namespace uh::cluster {

template <typename K, typename V> class lru_cache {

    struct Node {
        K key;
        V value;
    };

    std::unordered_map<K, typename std::list<Node>::iterator> m_map;
    std::list<Node> m_lruList;
    size_t m_capacity;
    std::mutex m;
    unsigned long m_hit{0};
    unsigned long m_total{1}; // to avoid conditions that handle divide by zero

public:
    explicit lru_cache(size_t capacity)
        : m_capacity{capacity} {}

    void put(const K& key, const V& value) { put(key, V(value)); }

    void put(const K& key, V&& value) {
        std::lock_guard<std::mutex> lock(m);
        if (const auto f = m_map.find(key); f != m_map.cend()) {
            f->second->value = std::forward<V>(value);
            m_lruList.splice(m_lruList.cend(), m_lruList, f->second);
        } else {
            if (m_map.size() >= m_capacity) {
                m_map.erase(m_lruList.front().key);
                m_lruList.pop_front();
            }
            m_lruList.emplace_back(key, std::forward<V>(value));
            m_map.emplace_hint(f, key, std::prev(m_lruList.end()));
        }
    }

    std::optional<std::reference_wrapper<const V>> get(const K& key) noexcept {
        std::lock_guard<std::mutex> lock(m);
        m_total++;
        if (const auto f = m_map.find(key); f != m_map.cend()) {
            m_hit++;
            m_lruList.splice(m_lruList.cend(), m_lruList, f->second);
            return f->second->value;
        }
        return std::nullopt;
    }

    V get(const K& key, V&& default_value) noexcept {
        std::lock_guard<std::mutex> lock(m);
        m_total++;
        if (const auto f = m_map.find(key); f != m_map.cend()) {
            m_hit++;
            m_lruList.splice(m_lruList.cend(), m_lruList, f->second);
            return f->second->value;
        }
        return default_value;
    }

    inline double get_hit_rate() noexcept {
        return static_cast<double>(m_hit) / m_total;
    }
};
} // end namespace uh::cluster

#endif // UH_CLUSTER_LRU_CACHE_H
