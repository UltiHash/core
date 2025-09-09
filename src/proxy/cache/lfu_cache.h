#pragma once

#include <proxy/cache/cache.h>

#include <list>
#include <mutex>
#include <numeric>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace uh::cluster::proxy::cache {

template <typename Key, EntryType Entry>
class lfu_cache : public cache_interface<Key, Entry> {
private:
    struct local_entry : Entry {
        size_t frequency{1};
        std::list<Key>::iterator freq_iterator;

        local_entry(Entry&& v, std::list<Key>::iterator it)
            : Entry(std::move(v)),
              freq_iterator{it} {}
    };

public:
    lfu_cache() = default;

    std::shared_ptr<Entry> get(const Key& key) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return nullptr;
        }
        increment_frequency(key);
        return it->second;
    }

    std::shared_ptr<Entry> remove(const Key& key) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            size_t freq = it->second->frequency;
            m_frequency_lists[freq].erase(it->second->freq_iterator);
            if (m_frequency_lists[freq].empty()) {
                m_frequency_lists.erase(freq);
                if (freq == m_min_frequency) {
                    m_min_frequency = m_frequency_lists.empty()
                                          ? 1
                                          : m_frequency_lists.begin()->first;
                }
            }

            auto ret = std::move(it->second);
            m_cache.erase(it);
            return ret;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Entry>> evict(std::size_t size) override {
        std::unique_lock lock(m_mutex);
        std::vector<std::shared_ptr<Entry>> ret;
        for (auto freq_it = m_frequency_lists.begin();
             freq_it != m_frequency_lists.end() && size > 0;) {
            auto& key_list = freq_it->second;
            for (auto key_it = key_list.rbegin();
                 key_it != key_list.rend() && size > 0;) {
                auto& key = *key_it;
                auto& item = m_cache[key];
                if (item.use_count() == 1) {
                    size_t value_size = item->data_size();
                    size = (value_size > size) ? 0 : size - value_size;

                    ret.push_back(std::move(item));
                    m_cache.erase(key);
                    key_it = std::reverse_iterator(
                        key_list.erase(std::next(key_it).base()));
                } else {
                    ++key_it;
                }
            }

            if (key_list.empty()) {
                freq_it = m_frequency_lists.erase(freq_it);
            } else {
                ++freq_it;
            }
        }
        if (m_frequency_lists.empty()) {
            m_min_frequency = 0;
        } else {
            m_min_frequency = m_frequency_lists.begin()->first;
        }
        return ret;
    }

    [[nodiscard]] std::shared_ptr<Entry> put(const Key& key,
                                             Entry&& entry) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        std::shared_ptr<Entry> old_entry = nullptr;

        m_frequency_lists[1].push_front(key);
        auto new_entry = std::make_shared<local_entry>(
            std::move(entry), m_frequency_lists[1].begin());

        if (it != m_cache.end()) {
            old_entry = std::move(it->second);
            it->second = std::move(new_entry);
        } else {
            m_cache[key] = std::move(new_entry);
        }
        m_min_frequency = 1;
        return old_entry;
    }

    size_t size() const override {
        std::shared_lock lock(m_mutex);
        return m_cache.size();
    }

private:
    std::unordered_map<size_t, std::list<Key>> m_frequency_lists;
    std::unordered_map<Key, std::shared_ptr<local_entry>> m_cache;

    size_t m_min_frequency = 0;

    mutable std::shared_mutex m_mutex;

    void increment_frequency(const Key& key) {
        auto& item = m_cache[key];
        size_t old_frequency = item->frequency;
        size_t new_frequency = old_frequency + 1;

        m_frequency_lists[old_frequency].erase(item->freq_iterator);
        if (m_frequency_lists[old_frequency].empty()) {
            m_frequency_lists.erase(old_frequency);
            if (old_frequency == m_min_frequency) {
                m_min_frequency = new_frequency;
            }
        }

        m_frequency_lists[new_frequency].push_front(key);
        item->frequency = new_frequency;
        item->freq_iterator = m_frequency_lists[new_frequency].begin();
    }
};

} // namespace uh::cluster::proxy::cache
