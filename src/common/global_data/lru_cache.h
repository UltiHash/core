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
    std::mutex m_mutex;

public:
    explicit lru_cache(size_t capacity)
        : m_capacity{capacity} {}

    /**
     * @brief Inserts a key-value pair into the cache.
     *
     * If the key already exists in the cache, updates its corresponding value.
     * If the key does not exist:
     *  - If the cache is full, removes the least recently used item.
     *  - Inserts the new key-value pair into the cache.
     *
     * @param key The key to insert or update.
     * @param value The rvalue reference to a key object.
     */
    void put(const K& key, V&& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
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

    /**
     * @brief Retrieves the value associated with the given key.
     *
     * If the key exists in the cache, moves it to the most recently used
     * position.
     *
     * @param key The key to search for in the cache.
     * @return An optional reference to the value associated with the key if
     * found; otherwise, std::nullopt.
     */
    std::optional<std::reference_wrapper<const V>> get(const K& key) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (const auto f = m_map.find(key); f != m_map.cend()) {
            m_lruList.splice(m_lruList.cend(), m_lruList, f->second);
            return f->second->value;
        }
        return std::nullopt;
    }

    /**
     * @brief Retrieves the value associated with the given key or a default
     * value.
     *
     * If the key exists in the cache, moves it to the most recently used
     * position. If the key does not exist, returns the provided default value.
     *
     * @param key The key to search for in the cache.
     * @param default_value The default value to return if the key is not found.
     * @return The value associated with the key if found; otherwise, the
     * provided default value.
     */
    V get(const K& key, V&& default_value) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (const auto f = m_map.find(key); f != m_map.cend()) {
            m_lruList.splice(m_lruList.cend(), m_lruList, f->second);
            return f->second->value;
        }
        return default_value;
    }
};
} // end namespace uh::cluster

#endif // UH_CLUSTER_LRU_CACHE_H
