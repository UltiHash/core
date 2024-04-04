#define BOOST_TEST_MODULE "big_int tests"

#include <atomic>
#include <boost/test/unit_test.hpp>
#include <common/global_data/lru_cache.h>
#include <iostream>
#include <thread>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

void concurrent_insertions(lru_cache<int, int>& cache, int start, int end) {
    for (int i = start; i <= end; ++i) {
        cache.put(i, i);
    }
}

void concurrent_retrievals(lru_cache<int, int>& cache, int start, int end,
                           std::atomic<int>& failure_count) {
    for (int i = start; i <= end; ++i) {
        auto value = cache.get(i);
        if (value.has_value() && value.value() != i) {
            ++failure_count;
        }
    }
}

BOOST_AUTO_TEST_CASE(lru_evict_outdated_entries) {
    lru_cache<int, int> cache(5);

    // insert initial item
    cache.put(0, 0);

    // insert n other items to provoke eviction
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(3, 3);
    cache.put(4, 4);
    cache.put(5, 5);

    // retrieval of initial item fails
    BOOST_CHECK(!cache.get(0).has_value());
}

BOOST_AUTO_TEST_CASE(lru_track_accessed_entries) {
    lru_cache<int, int> cache(5);

    // insert initial item
    cache.put(0, 0);

    // insert n-1 other items
    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(3, 3);
    cache.put(4, 4);

    // retrieve initial item
    BOOST_CHECK(cache.get(0).has_value());

    // insert nth entry
    cache.put(5, 5);

    // Retrieve element E_0 -> success
    BOOST_CHECK(cache.get(0).has_value());
}

BOOST_AUTO_TEST_CASE(lru_update_key) {
    lru_cache<int, int> cache(5);

    // insert initial items
    cache.put(1, 1);
    cache.put(2, 2);

    // update first item
    cache.put(1, 3);

    BOOST_CHECK(cache.get(1).has_value());
    BOOST_CHECK_EQUAL(cache.get(1).value(), 3);
}

BOOST_AUTO_TEST_CASE(lru_update_key_without_eviction) {
    lru_cache<int, int> cache(2);

    // insert initial items
    cache.put(1, 1);
    cache.put(2, 2);

    // update first item
    cache.put(1, 3);

    // make sure that updating the first item did not evict the second item
    BOOST_CHECK(cache.get(2).has_value());
    BOOST_CHECK_EQUAL(cache.get(2).value(), 2);
    BOOST_CHECK(cache.get(1).has_value());
    BOOST_CHECK_EQUAL(cache.get(1).value(), 3);
}

BOOST_AUTO_TEST_CASE(lru_empty_cache) {
    lru_cache<int, int> cache(5);

    // retrieval from empty cache fails
    BOOST_CHECK(!cache.get(0).has_value());
}

BOOST_AUTO_TEST_CASE(lru_single_element) {
    lru_cache<int, int> cache(5);

    // inserting and retrieve from a cache with a single element
    cache.put(1, 1);
    BOOST_CHECK(cache.get(1).has_value());
    BOOST_CHECK_EQUAL(cache.get(1).value(), 1);
}

BOOST_AUTO_TEST_CASE(lru_single_capacity) {
    lru_cache<int, int> cache(1);

    cache.put(1, 1);
    cache.put(2, 2);
    BOOST_CHECK(cache.get(1) == std::nullopt);
    BOOST_CHECK(cache.get(2).has_value());
    BOOST_CHECK_EQUAL(cache.get(2).value(), 2);
}

BOOST_AUTO_TEST_CASE(concurrent_insertion_and_retrieval) {
    const size_t n = 1000000;
    const size_t num_threads = 16;

    lru_cache<int, int> cache(n);

    std::atomic<int> failure_count(0);

    // create threads for concurrent insertion
    std::vector<std::jthread> insert_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        insert_threads.emplace_back(concurrent_insertions, std::ref(cache),
                                    i * (n / num_threads) + 1,
                                    (i + 1) * (n / num_threads));
    }

    // create threads for concurrent retrieval
    std::vector<std::jthread> retrieval_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        retrieval_threads.emplace_back(
            concurrent_retrievals, std::ref(cache), i * (n / num_threads) + 1,
            (i + 1) * (n / num_threads), std::ref(failure_count));
    }

    // check that we never got an invalid result
    BOOST_CHECK_EQUAL(failure_count, 0);
}

} // namespace uh::cluster
