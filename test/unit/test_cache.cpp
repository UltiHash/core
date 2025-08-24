#define BOOST_TEST_MODULE "cache tests"

#include "lfu_cache.h"
#include "lru_cache.h"

#include "test_config.h"

namespace uh::cluster {

struct s3_object_key {
    std::string bucket_name;
    std::string object_name;
    std::string version;

    bool operator==(const s3_object_key& other) const {
        return bucket_name == other.bucket_name &&
               object_name == other.object_name && version == other.version;
    }
};

} // namespace uh::cluster

template <> struct std::hash<uh::cluster::s3_object_key> {
    size_t operator()(const uh::cluster::s3_object_key& key) const {
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

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(a_lru_cache)

BOOST_AUTO_TEST_CASE(queries_for_added_items) {
    lru_cache<s3_object_key, std::vector<char>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};

    cache.put(key1, std::vector<char>{1, 2, 3, 4});

    BOOST_CHECK_EQUAL(cache.get_used_space(), 4);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK(cache.get(key1) != std::nullopt);
}

BOOST_AUTO_TEST_CASE(does_same_thing_with_shared_ptr_as_value) {
    lru_cache<s3_object_key, std::shared_ptr<std::vector<char>>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};

    cache.put(key1, std::make_shared<std::vector<char>>(
                        std::initializer_list<char>{1, 2, 3, 4}));

    BOOST_CHECK_EQUAL(cache.get_used_space(), 4);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK(cache.get(key1) != std::nullopt);
}

BOOST_AUTO_TEST_CASE(deletes_least_recently_used_items) {
    lru_cache<s3_object_key, std::vector<char>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    auto data3 = std::vector<char>{8, 9, 10, 11};
    cache.put(key1, std::vector<char>{1, 2, 3, 4});
    cache.put(key2, std::vector<char>{5, 6, 7});
    // Use Key1 to make it most recently used
    auto data = cache.access(key1);
    (void)data;

    cache.put(key3, data3);

    BOOST_CHECK_EQUAL(cache.get_used_space(), 8);
    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) != std::nullopt);
    BOOST_CHECK(cache.get(key2) == std::nullopt);
    BOOST_CHECK(cache.get(key3) != std::nullopt);
}

BOOST_AUTO_TEST_CASE(doesnt_handle_an_object_bigger_than_capacity) {
    lru_cache<s3_object_key, std::vector<char>> cache(10);
    cache.put({"bucket1", "object1", "v1"}, std::vector<char>{1, 2, 3, 4});
    s3_object_key big_key = {"bucket4", "big_object", "v1"};
    auto big_data = std::vector<char>();
    big_data.resize(20); // Bigger than cache capacity

    BOOST_CHECK_THROW(cache.put(big_key, big_data), std::exception);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_put) {
    lru_cache<s3_object_key, std::vector<int>> cache(1000);

    const int thread_count = 10;
    const int items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &cache]() {
            for (int i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};

                cache.put(key, {i});
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

    auto data = cache.access(last_key);
    BOOST_CHECK(data.has_value());
    BOOST_CHECK_EQUAL(data->size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_lfu_cache)

BOOST_AUTO_TEST_CASE(queries_for_added_items) {
    lfu_cache<s3_object_key, std::vector<char>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};

    cache.put(key1, std::vector<char>{1, 2, 3, 4});

    BOOST_CHECK_EQUAL(cache.get_used_space(), 4);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK(cache.get(key1));
}

BOOST_AUTO_TEST_CASE(does_same_with_shared_ptr_as_value) {
    lfu_cache<s3_object_key, std::shared_ptr<std::vector<char>>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};

    cache.put(key1, std::make_shared<std::vector<char>>(
                        std::initializer_list<char>{1, 2, 3, 4}));

    BOOST_CHECK_EQUAL(cache.get_used_space(), 4);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK(cache.get(key1));
}

BOOST_AUTO_TEST_CASE(deletes_least_frequently_used_items) {
    lfu_cache<s3_object_key, std::vector<char>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    cache.put(key1, std::vector<char>{1, 2, 3, 4});
    cache.access(key1);
    cache.access(key1);
    cache.put(key2, std::vector<char>{5, 6, 7});
    cache.access(key2);

    auto data3 = std::vector<char>{8, 9, 10, 11};
    cache.put(key3, data3);

    BOOST_CHECK_EQUAL(cache.get_used_space(), 8);
    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) != std::nullopt);
    BOOST_CHECK(cache.get(key2) == std::nullopt);
    BOOST_CHECK(cache.get(key3) != std::nullopt);
}

BOOST_AUTO_TEST_CASE(doesnt_handle_an_object_bigger_than_capacity) {
    lfu_cache<s3_object_key, std::vector<char>> cache(10);
    cache.put({"bucket1", "object1", "v1"}, std::vector<char>{1, 2, 3, 4});
    s3_object_key big_key = {"bucket4", "big_object", "v1"};
    auto big_data = std::vector<char>();
    big_data.resize(20); // Bigger than cache capacity

    BOOST_CHECK_THROW(cache.put(big_key, big_data), std::exception);
}

BOOST_AUTO_TEST_CASE(evicts_oldest_item_when_frequencies_are_equal) {
    lfu_cache<s3_object_key, std::vector<char>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    cache.put(key1, std::vector<char>{1, 2, 3});
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    cache.put(key2, std::vector<char>{4, 5, 6});
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    cache.put(key3, std::vector<char>{10, 11, 12, 13, 14, 15, 16});

    BOOST_CHECK(cache.get(key1) == std::nullopt);
    BOOST_CHECK(cache.get(key2) != std::nullopt);
    BOOST_CHECK(cache.get(key3) != std::nullopt);
}

BOOST_AUTO_TEST_CASE(resets_frequency_when_updating_existing_key) {
    lfu_cache<s3_object_key, std::vector<char>> cache(10);
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    cache.put(key1, std::vector<char>{1, 2, 3});
    cache.access(key1);
    cache.access(key1);
    cache.put(key2, std::vector<char>{4, 5, 6});
    cache.access(key1);
    cache.put(key1, std::vector<char>{7, 8, 9});

    s3_object_key key3 = {"bucket3", "object3", "v3"};
    cache.put(key3, std::vector<char>{10, 11, 12, 13, 14, 15, 16});

    BOOST_CHECK(cache.get(key1) != std::nullopt);
    BOOST_CHECK(cache.get(key2) == std::nullopt);
    BOOST_CHECK(cache.get(key3) != std::nullopt);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_put) {
    lfu_cache<s3_object_key, std::vector<int>> cache(1000);

    const int thread_count = 10;
    const int items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &cache]() {
            for (int i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};

                cache.put(key, {i});
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

    auto data = cache.access(last_key);
    BOOST_CHECK(data.has_value());
    BOOST_CHECK_EQUAL(data->size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace uh::cluster
