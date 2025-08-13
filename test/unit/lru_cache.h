#pragma once

#include "cache.h"

#include <list>
#include <mutex>
#include <numeric>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace uh::cluster {

template <typename Key, typename Value>
class lru_cache : public cache_base<Key, Value> {
private:
    std::size_t m_capacity;
    std::size_t m_current_size = 0;
    std::list<std::pair<Key, Value>> m_items;

    using list_iterator = typename std::list<std::pair<Key, Value>>::iterator;
    std::unordered_map<Key, list_iterator> m_cache;

    mutable std::shared_mutex m_mutex;

public:
    explicit lru_cache(std::size_t capacity)
        : m_capacity(capacity) {}

    void touch(const Key& key) {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            throw std::range_error("Key not found in cache");
        }

        m_items.splice(m_items.begin(), m_items, it->second);
    }

    std::optional<Value> access(const Key& key) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return std::nullopt;
        }

        m_items.splice(m_items.begin(), m_items, it->second);
        return it->second->second;
    }

    std::optional<Value> get(const Key& key) const override {
        std::shared_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return std::nullopt;
        }

        return it->second->second;
    }

    void put(const Key& key, const Value& value) override {
        std::unique_lock lock(m_mutex);

        std::size_t value_size = value.size();
        if (value_size > m_capacity) {
            throw std::length_error("Value size exceeds cache capacity");
        }

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            m_current_size -= it->second->second.size();
            m_items.erase(it->second);
            m_cache.erase(it);
        }

        while (!m_items.empty() && (m_current_size + value_size > m_capacity)) {
            m_current_size -= m_items.back().second.size();
            m_cache.erase(m_items.back().first);
            m_items.pop_back();
        }

        m_items.emplace_front(key, value);
        m_cache[key] = m_items.begin();
        m_current_size += value_size;
    }

    std::size_t size() const override {
        std::shared_lock lock(m_mutex);
        return m_cache.size();
    }

    std::size_t get_used_space() const override {
        std::shared_lock lock(m_mutex);
        return m_current_size;
    }

    void clear() override {
        std::unique_lock lock(m_mutex);
        m_items.clear();
        m_cache.clear();
        m_current_size = 0;
    }
};

} // namespace uh::cluster
