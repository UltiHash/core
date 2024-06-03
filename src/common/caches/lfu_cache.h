#ifndef UH_CLUSTER_LFU_CACHE_H
#define UH_CLUSTER_LFU_CACHE_H

#include <list>
#include <map>
#include <optional>
#include <functional>

namespace uh::cluster {

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
        explicit lfu_cache(size_t capacity, std::optional<std::function<void(Key, Value)>> removal_callback =
                std::nullopt)
            : m_capacity{capacity},
              m_removal_callback{std::move(removal_callback)} {}

        std::optional<std::reference_wrapper<Value>> get(const Key& key) {
            auto it = m_lookup.find(key);
            if (it == m_lookup.end())
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

} // end namespace uh::cluster

#endif // UH_CLUSTER_LFU_CACHE_H