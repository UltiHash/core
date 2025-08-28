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

template <typename Key, typename Entry>
class lru_cache : public cache_interface<Key, Entry> {
public:
    lru_cache() = default;

    using time_point = typename entry_interface<Entry>::time_point;

    [[nodiscard]] std::shared_ptr<Entry> get(const Key& key) override {
        std::shared_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return nullptr;
        }
        m_items.splice(m_items.begin(), m_items, it->second);
        return it->second->second;
    }

    [[nodiscard]] std::shared_ptr<Entry> remove(const Key& key) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            auto ret = std::move(it->second->second);
            m_items.erase(it->second);
            m_cache.erase(it);
            return ret;
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<std::shared_ptr<Entry>>
    evict(std::size_t size,
          std::optional<time_point> expire_before = std::nullopt) override {
        std::unique_lock lock(m_mutex);
        std::vector<std::shared_ptr<Entry>> ret;
        for (auto it = m_items.rbegin(); it != m_items.rend() && size > 0;) {
            if (it->second.use_count() == 1 &&
                (!expire_before.has_value() ||
                 it->second->get_expire_time() < expire_before.value())) {
                auto t = it->second->size();
                size = (t > size) ? 0 : size - t;
                m_cache.erase(it->first);
                ret.push_back(std::move(it->second));
                it = std::reverse_iterator(m_items.erase(std::next(it).base()));
            } else {
                ++it;
            }
        }
        return ret;
    }

    [[nodiscard]] std::shared_ptr<Entry>
    put(const Key& key, std::shared_ptr<Entry> entry) override {
        std::unique_lock lock(m_mutex);
        auto it = m_cache.find(key);
        std::shared_ptr<Entry> old_entry = nullptr;

        m_items.emplace_front(key, std::move(entry));
        if (it != m_cache.end()) {
            old_entry = std::move(it->second->second);
            it->second = m_items.begin();
        } else {
            m_cache[key] = m_items.begin();
        }
        return old_entry;
    }

    std::size_t size() const override {
        std::shared_lock lock(m_mutex);
        return m_cache.size();
    }

private:
    using items_t = std::list<std::pair<Key, std::shared_ptr<Entry>>>;
    items_t m_items;

    std::unordered_map<Key, typename items_t::iterator> m_cache;

    mutable std::shared_mutex m_mutex;
};

} // namespace uh::cluster::proxy::cache
