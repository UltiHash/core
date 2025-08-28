#define BOOST_TEST_MODULE "cache tests"

#include <proxy/cache/lfu_cache.h>
#include <proxy/cache/lru_cache.h>

#include "test_config.h"

namespace uh::cluster::proxy::cache {

struct s3_object_key {
    std::string bucket_name;
    std::string object_name;
    std::string version;

    bool operator==(const s3_object_key& other) const {
        return bucket_name == other.bucket_name &&
               object_name == other.object_name && version == other.version;
    }
};

} // namespace uh::cluster::proxy::cache

template <> struct std::hash<uh::cluster::proxy::cache::s3_object_key> {
    size_t
    operator()(const uh::cluster::proxy::cache::s3_object_key& key) const {
        std::size_t seed = 0;

        auto hash_combine = [](std::size_t& seed, const auto& v) {
            seed ^= std::hash<std::remove_cvref_t<decltype(v)>>{}(v) +
                    0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

        hash_combine(seed, key.bucket_name);
        hash_combine(seed, key.object_name);
        hash_combine(seed, key.version);

        return seed;
    }
};

namespace uh::cluster::proxy::cache {

struct entry : public entry_interface<entry> {
    std::vector<int> value;

    entry(std::vector<int>&& v)
        : value(std::move(v)) {}

    std::size_t size() const { return value.size(); }

    int& operator[](std::size_t i) { return value[i]; }
    const int& operator[](std::size_t i) const { return value[i]; }

    static std::shared_ptr<entry> create(std::initializer_list<int> il) {
        return std::make_shared<entry>(std::vector<int>(il));
    }
};

BOOST_AUTO_TEST_SUITE(a_lru_cache)

BOOST_AUTO_TEST_CASE(queries_for_added_items) {
    auto cache = lru_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    // NOTE: (void) to suppress unused variable warning, since cache assumes it
    // stores value's handle rather than value itself, which should be deleted
    // seperately
    (void)cache.put(key1, entry::create({1, 2, 3, 4}));

    auto pe = cache.get(key1);

    BOOST_CHECK(pe != nullptr);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK((*pe)[0] == 1);
    BOOST_CHECK((*pe)[3] == 4);
    BOOST_CHECK((*pe).size() == 4);
}

BOOST_AUTO_TEST_CASE(deletes_least_recently_used_items) {
    auto cache = lru_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    auto data3 = entry::create({8, 9, 10, 11});
    (void)cache.put(key1, entry::create({1, 2, 3, 4}));
    (void)cache.put(key2, entry::create({5, 6, 7}));
    // Use Key1 to make it most recently used
    auto pe = cache.get(key1);
    (void)pe;
    (void)cache.put(key3, data3);
    (void)cache.evict(1);

    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) != nullptr);
    BOOST_CHECK(cache.get(key2) == nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
}

BOOST_AUTO_TEST_CASE(returns_previous_value_on_put_when_key_exists) {
    auto cache = lru_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, entry::create({1, 2, 3}));

    auto pe = cache.put(key1, entry::create({10, 11}));
    BOOST_CHECK(pe != nullptr);
}

BOOST_AUTO_TEST_CASE(resets_usage_when_updating_existing_key) {
    auto cache = lru_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    (void)cache.put(key1, entry::create({1, 2, 3}));
    (void)cache.get(key1);
    (void)cache.put(key2, entry::create({4, 5, 6}));
    (void)cache.get(key2);
    (void)cache.put(key2, entry::create({7, 8, 9}));

    (void)cache.evict(1);

    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_put) {
    auto cache = lru_cache<s3_object_key, entry>();

    const int thread_count = 10;
    const int items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &cache]() {
            for (int i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};

                (void)cache.put(key, entry::create({i}));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    BOOST_CHECK_EQUAL(cache.size(), 1000);

    s3_object_key last_key{"bucket" + std::to_string(thread_count - 1),
                           "object" + std::to_string(items_per_thread - 1),
                           "v1"};

    auto pe = cache.get(last_key);
    BOOST_CHECK(pe != nullptr);
    BOOST_CHECK_EQUAL((*pe).size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_lfu_cache)

BOOST_AUTO_TEST_CASE(queries_for_added_items) {
    auto cache = lfu_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, entry::create({1, 2, 3, 4}));

    auto pe = cache.get(key1);

    BOOST_CHECK(pe != nullptr);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK((*pe)[0] == 1);
    BOOST_CHECK((*pe)[3] == 4);
    BOOST_CHECK((*pe).size() == 4);
}

BOOST_AUTO_TEST_CASE(deletes_least_frequently_used_items) {
    auto cache = lfu_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key1, entry::create({1, 2, 3, 4}));
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.put(key2, entry::create({5, 6, 7}));
    (void)cache.get(key2);
    auto data3 = entry::create({8, 9, 10, 11});
    (void)cache.put(key3, data3);

    cache.evict(1);

    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) != nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
    BOOST_CHECK(cache.get(key3) == nullptr);
}

BOOST_AUTO_TEST_CASE(evicts_oldest_item_when_frequencies_are_equal) {
    auto cache = lfu_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, entry::create({1, 2, 3}));
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    (void)cache.put(key2, entry::create({4, 5, 6}));
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key3, entry::create({10, 11}));

    cache.evict(4);

    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) == nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
}

BOOST_AUTO_TEST_CASE(returns_previous_value_on_put_when_key_exists) {
    auto cache = lru_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, entry::create({1, 2, 3}));

    auto pe = cache.put(key1, entry::create({10, 11}));
    BOOST_CHECK(pe != nullptr);
}

BOOST_AUTO_TEST_CASE(resets_frequency_when_updating_existing_key) {
    auto cache = lfu_cache<s3_object_key, entry>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    (void)cache.put(key1, entry::create({1, 2, 3}));
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.put(key2, entry::create({4, 5, 6}));
    (void)cache.get(key2);
    (void)cache.put(key1, entry::create({7, 8, 9}));
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key3, entry::create({10, 11}));

    cache.evict(1);

    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_put) {
    auto cache = lfu_cache<s3_object_key, entry>();

    const int thread_count = 10;
    const int items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &cache]() {
            for (int i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};

                (void)cache.put(key, entry::create({i}));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    BOOST_CHECK_EQUAL(cache.size(), 1000);

    s3_object_key last_key{"bucket" + std::to_string(thread_count - 1),
                           "object" + std::to_string(items_per_thread - 1),
                           "v1"};

    auto pe = cache.get(last_key);
    BOOST_CHECK(pe != nullptr);
    BOOST_CHECK_EQUAL((*pe).size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache
