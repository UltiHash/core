#define BOOST_TEST_MODULE "lfu tests"

#include "common/caches/lfu_cache.h"
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(lfu_evict_outdated_entries) {
    lfu_cache<int, int> cache(5);

    cache.put_non_existing(0, 0);

    cache.put_non_existing(1, 1);
    cache.put_non_existing(2, 2);
    cache.put_non_existing(3, 3);
    cache.put_non_existing(4, 4);
    cache.put_non_existing(5, 5);

    BOOST_CHECK(!cache.get(0).has_value());
}

BOOST_AUTO_TEST_CASE(lfu_track_accessed_entries) {
    lfu_cache<int, int> cache(5);

    cache.put_non_existing(0, 0);

    cache.put_non_existing(1, 1);
    cache.put_non_existing(2, 2);
    cache.put_non_existing(3, 3);
    cache.put_non_existing(4, 4);

    BOOST_CHECK(cache.get(0).has_value());

    cache.put_non_existing(5, 5);

    BOOST_CHECK(cache.get(0).has_value());
}

BOOST_AUTO_TEST_CASE(lfu_track_freq_of_entries) {
    lfu_cache<int, int> cache(5);

    cache.put_non_existing(0, 0);

    cache.put_non_existing(1, 1);
    cache.put_non_existing(2, 2);
    cache.put_non_existing(3, 3);
    cache.put_non_existing(4, 4);

    cache.use(0);
    cache.use(1);
    cache.use(3);
    cache.use(4);

    cache.put_non_existing(5, 5);

    BOOST_CHECK(cache.get(0).has_value());
    BOOST_CHECK(cache.get(1).has_value());
    BOOST_CHECK(!cache.get(2).has_value());
    BOOST_CHECK(cache.get(3).has_value());
    BOOST_CHECK(cache.get(4).has_value());
    BOOST_CHECK(cache.get(5).has_value());
}

BOOST_AUTO_TEST_CASE(lfu_empty_cache) {
    lfu_cache<int, int> cache(5);

    BOOST_CHECK(!cache.get(0).has_value());
}

BOOST_AUTO_TEST_CASE(lfu_single_element) {
    lfu_cache<int, int> cache(5);

    cache.put_non_existing(1, 1);
    BOOST_CHECK(cache.get(1).has_value());
    BOOST_CHECK_EQUAL(cache.get(1).value(), 1);
}

BOOST_AUTO_TEST_CASE(lfu_single_capacity) {
    lfu_cache<int, int> cache(1);

    cache.put_non_existing(1, 1);
    cache.put_non_existing(2, 2);
    BOOST_CHECK(cache.get(1) == std::nullopt);
    BOOST_CHECK(cache.get(2).has_value());
    BOOST_CHECK_EQUAL(cache.get(2).value(), 2);
}

BOOST_AUTO_TEST_CASE(lfu_erase) {
    lfu_cache<int, int> cache(5);
    cache.put_non_existing(0, 0);
    cache.put_non_existing(1, 1);
    cache.put_non_existing(2, 2);
    cache.put_non_existing(3, 3);
    cache.put_non_existing(4, 4);
    cache.erase(4);
    BOOST_CHECK(!cache.get(4).has_value());
    cache.put_non_existing(5, 5);

    BOOST_CHECK(cache.get(0).has_value());
}

} // namespace uh::cluster
