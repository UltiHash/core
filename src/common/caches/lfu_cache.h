#ifndef UH_CLUSTER_LFU_CACHE_H
#define UH_CLUSTER_LFU_CACHE_H

#include <forward_list>
#include <functional>
#include <list>
#include <map>
#include <optional>

namespace uh::cluster {

template <typename Key, typename Value> class lfu_cache {

    struct freq_bucket {
        const size_t freq;
        std::list<Key> items;
        explicit freq_bucket(size_t fq)
            : freq{fq} {}
    };

    struct key_data {
        const Value val;
        std::list<freq_bucket>::iterator bucket;
        std::list<Key>::const_iterator pos;
    };

    size_t m_capacity;
    std::optional<std::function<void(Key, Value)>> m_removal_callback;

    std::unordered_map<Key, key_data> m_key_data;
    std::list<freq_bucket> m_freq_buckets;

public:
    explicit lfu_cache(size_t capacity,
                       std::optional<std::function<void(Key, Value)>>
                           removal_callback = std::nullopt)
        : m_capacity{capacity},
          m_removal_callback{std::move(removal_callback)} {}

    inline void put_non_existing(const Key& key, Value&& val) {

        if (m_freq_buckets.front().freq != 1) {
            m_freq_buckets.emplace_front(1);
        }

        auto first_bucket = m_freq_buckets.begin();
        first_bucket->items.emplace_back(key);

        m_key_data.emplace(key,
                           key_data{
                               .val = std::forward<Value>(val),
                               .bucket = first_bucket,
                               .pos = std::prev(first_bucket->items.cend()),
                           });
        if (m_capacity == 0) {
            auto& front_list = m_freq_buckets.front().items;
            const auto& rem_key = front_list.front();
            auto rem_itr = m_key_data.find(rem_key);
            if (m_removal_callback) {
                (*m_removal_callback)(rem_key, rem_itr->second.val);
            }

            m_key_data.erase(rem_itr);
            front_list.pop_front();
            if (front_list.empty()) {
                m_freq_buckets.pop_front();
            }
        } else {
            m_capacity--;
        }
    }

    inline void use(const Key& key) {
        if (auto itr = m_key_data.find(key); itr != m_key_data.cend()) {
            increment(itr);
        }
    }

    inline std::optional<Value> get(const Key& key) {
        if (auto itr = m_key_data.find(key); itr != m_key_data.cend()) {
            increment(itr);
            return itr->second.val;
        }
        return {};
    }

private:
    inline void increment(auto& itr) {

        auto& bucket_itr = itr->second.bucket;
        auto& key = itr->first;
        const auto new_freq = bucket_itr->freq + 1;
        auto next_bucket = std::next(bucket_itr);

        // if a bucket with the desired frequency does not exist
        if (next_bucket == m_freq_buckets.cend() or
            next_bucket->freq != new_freq) {
            next_bucket = m_freq_buckets.emplace(next_bucket, new_freq);
        }

        next_bucket->items.emplace_back(key);
        bucket_itr->items.erase(itr->second.pos);
        if (bucket_itr->items.empty()) {
            m_freq_buckets.erase(bucket_itr);
        }

        itr->second.pos = std::prev(next_bucket->items.cend());
        bucket_itr = next_bucket;
    }
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_LFU_CACHE_H