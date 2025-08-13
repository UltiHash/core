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
class lfu_cache : public cache_base<Key, Value> {
private:
    struct CacheItem {
        Value value;
        size_t frequency; // to reference back in O(1)
        std::list<Key>::iterator freq_iterator;
    };

    size_t m_capacity;
    size_t m_current_size = 0;

    std::unordered_map<size_t, std::list<Key>> m_frequency_lists;
    std::unordered_map<Key, CacheItem> m_cache;

    size_t m_min_frequency = 0;

    mutable std::shared_mutex m_mutex;

    void increment_frequency(const Key& key) {
        auto& item = m_cache[key];
        size_t old_frequency = item.frequency;
        size_t new_frequency = old_frequency + 1;

        m_frequency_lists[old_frequency].erase(item.freq_iterator);
        if (m_frequency_lists[old_frequency].empty()) {
            m_frequency_lists.erase(old_frequency);
            if (old_frequency == m_min_frequency) {
                m_min_frequency = new_frequency;
            }
        }

        m_frequency_lists[new_frequency].push_front(key);
        item.frequency = new_frequency;
        item.freq_iterator = m_frequency_lists[new_frequency].begin();
    }

    void evict_if_needed(size_t needed_space) {
        while (!m_cache.empty() &&
               (m_current_size + needed_space > m_capacity)) {
            auto& min_freq_list = m_frequency_lists[m_min_frequency];
            Key key_to_evict = min_freq_list.back();

            size_t value_size = m_cache[key_to_evict].value.size();
            m_current_size -= value_size;

            min_freq_list.pop_back();
            if (min_freq_list.empty()) {
                m_frequency_lists.erase(m_min_frequency);

                if (!m_frequency_lists.empty()) {
                    m_min_frequency = std::numeric_limits<size_t>::max();
                    for (const auto& [freq, _] : m_frequency_lists) {
                        m_min_frequency = std::min(m_min_frequency, freq);
                    }
                }
            }

            m_cache.erase(key_to_evict);
        }
    }

public:
    explicit lfu_cache(size_t capacity)
        : m_capacity(capacity) {}

    std::optional<Value> access(const Key& key) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return std::nullopt;
        }

        Value result = it->second.value;
        increment_frequency(key);
        return result;
    }

    std::optional<Value> get(const Key& key) const override {
        std::shared_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return std::nullopt;
        }

        return it->second.value;
    }

    void put(const Key& key, const Value& value) override {
        std::unique_lock lock(m_mutex);

        size_t value_size = value.size();
        if (value_size > m_capacity) {
            throw std::length_error("Value size exceeds cache capacity");
        }

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            size_t old_freq = it->second.frequency;
            m_frequency_lists[old_freq].erase(it->second.freq_iterator);
            if (m_frequency_lists[old_freq].empty()) {
                m_frequency_lists.erase(old_freq);
                if (old_freq == m_min_frequency) {
                    m_min_frequency = m_frequency_lists.empty()
                                          ? 1
                                          : m_frequency_lists.begin()->first;
                }
            }

            m_current_size -= it->second.value.size();
            m_cache.erase(it);
        }

        evict_if_needed(value_size);

        m_frequency_lists[1].push_front(key);
        m_cache[key] = {std::move(value), 1, m_frequency_lists[1].begin()};
        m_current_size += value_size;
        m_min_frequency = 1;
    }

    size_t size() const override {
        std::shared_lock lock(m_mutex);
        return m_cache.size();
    }

    size_t get_used_space() const override {
        std::shared_lock lock(m_mutex);
        return m_current_size;
    }

    void clear() override {
        std::unique_lock lock(m_mutex);
        m_frequency_lists.clear();
        m_cache.clear();
        m_current_size = 0;
        m_min_frequency = 0;
    }
};

} // namespace uh::cluster
