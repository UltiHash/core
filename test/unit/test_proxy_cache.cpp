// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define BOOST_TEST_MODULE "cache tests"

#include <util/proxy_cache_config.h>

#include <proxy/cache/lfu_cache.h>
#include <proxy/cache/lru_cache.h>

#include "test_config.h"

namespace uh::cluster::proxy::cache {

BOOST_AUTO_TEST_SUITE(a_lru_cache)

BOOST_AUTO_TEST_CASE(queries_for_added_items) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    // NOTE: (void) to suppress unused variable warning, since cache assumes it
    // stores value's handle rather than value itself, which should be deleted
    // seperately
    (void)cache.put(key1, char_vector({1, 2, 3, 4}));

    auto pe = cache.get(key1);

    BOOST_CHECK(pe != nullptr);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK((*pe)[0] == 1);
    BOOST_CHECK((*pe)[3] == 4);
    BOOST_CHECK((*pe).data_size() == 4);
}

BOOST_AUTO_TEST_CASE(evicts_least_recently_used_items) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key1, char_vector({1, 2, 3, 4}));
    (void)cache.put(key2, char_vector({5, 6, 7}));
    // Use Key1 to make it most recently used
    (void)cache.get(key1);
    (void)cache.put(key3, char_vector({8, 9, 10, 11}));
    (void)cache.evict(1);

    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) != nullptr);
    BOOST_CHECK(cache.get(key2) == nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
}

BOOST_AUTO_TEST_CASE(evicts_except_items_in_use) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    s3_object_key key4 = {"bucket4", "object4", "v4"};
    (void)cache.put(key1, char_vector({1, 2, 3, 4}));
    (void)cache.put(key2, char_vector({5, 6, 7}));
    (void)cache.put(key3, char_vector({8, 9, 10}));
    (void)cache.put(key4, char_vector({11}));
    auto pe2 = cache.get(key2);
    auto pe3 = cache.get(key3);
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.evict(5);

    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
    BOOST_CHECK(cache.get(key4) == nullptr);
}

BOOST_AUTO_TEST_CASE(returns_previous_value_on_put_when_key_exists) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, char_vector({1, 2, 3}));

    auto pe = cache.put(key1, char_vector({10, 11}));
    BOOST_CHECK(pe != nullptr);
}

BOOST_AUTO_TEST_CASE(resets_usage_when_updating_existing_key) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    (void)cache.put(key1, char_vector({1, 2, 3}));
    (void)cache.get(key1);
    (void)cache.put(key2, char_vector({4, 5, 6}));
    (void)cache.get(key2);
    (void)cache.put(key2, char_vector({7, 8, 9}));

    (void)cache.evict(1);

    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_put) {
    auto cache = lru_cache<s3_object_key, char_vector>();

    const int thread_count = 10;
    const char items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &cache]() {
            for (char i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};

                (void)cache.put(key, char_vector({i}));
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
    BOOST_CHECK_EQUAL((*pe).data_size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_lfu_cache)

BOOST_AUTO_TEST_CASE(queries_for_added_items) {
    auto cache = lfu_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, char_vector({1, 2, 3, 4}));

    auto pe = cache.get(key1);

    BOOST_CHECK(pe != nullptr);
    BOOST_CHECK_EQUAL(cache.size(), 1);
    BOOST_CHECK((*pe)[0] == 1);
    BOOST_CHECK((*pe)[3] == 4);
    BOOST_CHECK((*pe).data_size() == 4);
}

BOOST_AUTO_TEST_CASE(evicts_least_frequently_used_items) {
    auto cache = lfu_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key1, char_vector({1, 2, 3, 4}));
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.put(key2, char_vector({5, 6, 7}));
    (void)cache.get(key2);
    (void)cache.put(key3, char_vector({8, 9, 10, 11}));

    cache.evict(1);

    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) != nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
    BOOST_CHECK(cache.get(key3) == nullptr);
}

BOOST_AUTO_TEST_CASE(evicts_oldest_item_when_frequencies_are_equal) {
    auto cache = lfu_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, char_vector({1, 2, 3}));
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    (void)cache.put(key2, char_vector({4, 5, 6}));
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key3, char_vector({10, 11}));

    cache.evict(4);

    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) == nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
}

BOOST_AUTO_TEST_CASE(evicts_except_items_in_use) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    s3_object_key key4 = {"bucket4", "object4", "v4"};
    (void)cache.put(key1, char_vector({1, 2, 3, 4}));
    (void)cache.put(key2, char_vector({5, 6, 7}));
    (void)cache.put(key3, char_vector({8, 9, 10}));
    (void)cache.put(key4, char_vector({11}));
    auto pe2 = cache.get(key2);
    auto pe3 = cache.get(key3);
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.evict(5);

    BOOST_CHECK_EQUAL(cache.size(), 2);
    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
    BOOST_CHECK(cache.get(key4) == nullptr);
}

BOOST_AUTO_TEST_CASE(returns_previous_value_on_put_when_key_exists) {
    auto cache = lru_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    (void)cache.put(key1, char_vector({1, 2, 3}));

    auto pe = cache.put(key1, char_vector({10, 11}));
    BOOST_CHECK(pe != nullptr);
}

BOOST_AUTO_TEST_CASE(resets_frequency_when_updating_existing_key) {
    auto cache = lfu_cache<s3_object_key, char_vector>();
    s3_object_key key1 = {"bucket1", "object1", "v1"};
    s3_object_key key2 = {"bucket2", "object2", "v2"};
    (void)cache.put(key1, char_vector({1, 2, 3}));
    (void)cache.get(key1);
    (void)cache.get(key1);
    (void)cache.put(key2, char_vector({4, 5, 6}));
    (void)cache.get(key2);
    (void)cache.put(key1, char_vector({7, 8, 9}));
    s3_object_key key3 = {"bucket3", "object3", "v3"};
    (void)cache.put(key3, char_vector({10, 11}));

    cache.evict(1);

    BOOST_CHECK(cache.get(key1) == nullptr);
    BOOST_CHECK(cache.get(key2) != nullptr);
    BOOST_CHECK(cache.get(key3) != nullptr);
}

BOOST_AUTO_TEST_CASE(supports_concurrent_put) {
    auto cache = lfu_cache<s3_object_key, char_vector>();

    const int thread_count = 10;
    const char items_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &cache]() {
            for (char i = 0; i < items_per_thread; ++i) {
                s3_object_key key{"bucket" + std::to_string(t),
                                  "object" + std::to_string(i), "v1"};

                (void)cache.put(key, char_vector({i}));
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
    BOOST_CHECK_EQUAL((*pe).data_size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::proxy::cache
