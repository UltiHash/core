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

        if (m_freq_buckets.begin()->freq != 1) {
            m_freq_buckets.emplace_front(1);
        }

        auto first_bucket = m_freq_buckets.begin();
        first_bucket->items.emplace_back(key);

        key_data k_data{
            .val = std::forward<Value>(val),
            .bucket = first_bucket,
            .pos = std::prev(first_bucket->items.cend()),
        };

        m_key_data.emplace(key, std::move(k_data));
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
            m_capacity++;
        }
        m_capacity--;
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

/*
template <typename Key, typename Value> class lfu_cache {

    struct freq_bucket {
        int freq;
        std::list<Key> keys;
    };

    struct item {
        Value value;
        std::list<freq_bucket>::iterator bucket;
        std::list<Key>::iterator position;
    };

    public:
        explicit lfu_cache(size_t capacity,
std::optional<std::function<void(Key, Value)>> removal_callback = std::nullopt)
            : m_capacity{capacity},
              m_removal_callback{std::move(removal_callback)} {}

        std::optional<std::reference_wrapper<Value>> get(const Key& key) {
            auto it = m_lookup.find(key);
            if (it == m_lookup.cend())
                return std::nullopt;
            m_lookup[key] = increment(key, it->second.value);
            return it->second.value;
        }

        inline void use(const Key& key) {
            if (auto it = m_lookup.find(key); it != m_lookup.end())
                m_lookup[key] = increment(key, it->second.value);
        }

        inline void put(const Key& key, Value&& val) {
            m_lookup[key] = increment(key, std::forward<Value>(val));
        }

    private:
        item increment(const Key& key, const Value& val) {
            auto it = m_lookup.find(key);
            if (it == m_lookup.end()) {
                if (m_capacity == 0) {
                    drop_least();
                    ++m_capacity;
                }
                auto bucket = get_one_bucket();
                bucket->keys.push_back(key);
                --m_capacity;
                return {val, bucket, std::prev(bucket->keys.end())};
            } else {
                auto bucket = get_next_bucket(it->second.bucket);
                bucket->keys.push_back(key);
                it->second.bucket->keys.erase(it->second.position);
                maybe_drop_bucket(it->second.bucket);
                return {val, bucket, std::prev(bucket->keys.end())};
            }
        }

        std::list<freq_bucket>::iterator get_one_bucket() {
            if (m_buckets.empty() || m_buckets.front().freq != 1) {
                m_buckets.push_front({1,{}});
            }
            return m_buckets.begin();
        }

        std::list<freq_bucket>::iterator
        get_next_bucket(std::list<freq_bucket>::iterator curr) {
            auto next = std::next(curr);
            if (next == m_buckets.end() || curr->freq + 1 < next->freq) {
                next = m_buckets.insert(next, {curr->freq + 1,{}});
            }
            return next;
        }

        void maybe_drop_bucket(std::list<freq_bucket>::iterator bucket) {
            if (bucket->keys.empty())
                m_buckets.erase(bucket);
        }

        void drop_least() {
            auto& key = m_buckets.front().keys.front();
            m_buckets.front().keys.pop_front();

            auto itr = m_lookup.find(key);
            if (m_removal_callback) {
                (*m_removal_callback)(key,
                                      itr->second.value);
            }
            m_lookup.erase(itr);
            maybe_drop_bucket(m_buckets.begin());
        }

        size_t m_capacity;
        std::optional<std::function<void(Key, Value)>> m_removal_callback;

        std::unordered_map<Key, item> m_lookup;
        std::list<freq_bucket> m_buckets;
    };
*/
} // end namespace uh::cluster

#endif // UH_CLUSTER_LFU_CACHE_H