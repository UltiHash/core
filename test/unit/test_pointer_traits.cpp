#define BOOST_TEST_MODULE "pointer_traits tests"

#include "test_config.h"

#include <boost/test/unit_test.hpp>
#include <common/utils/common.h>
#include <common/utils/pointer_traits.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(random_test) {
    int failures = 0;
    for (int i = 0; i < 100; ++i) {
        const auto storage_id = rand() % std::numeric_limits<uint32_t>::max();
        const auto storage_ptr = rand() % std::numeric_limits<uint64_t>::max();
        auto p =
            pointer_traits::rr::get_global_pointer(storage_ptr, 0, storage_id);

        auto [id, ptr] = pointer_traits::rr::get_storage_pointer(p);

        if (id != storage_id or ptr != storage_ptr) {
            failures++;
        }
    }
    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(edge_case_test) {
    auto storage_id = std::numeric_limits<uint32_t>::max();
    auto storage_ptr = std::numeric_limits<uint64_t>::max();
    auto p = pointer_traits::rr::get_global_pointer(storage_ptr, 0, storage_id);

    {
        auto [id, ptr] = pointer_traits::rr::get_storage_pointer(p);

        BOOST_TEST(ptr == storage_ptr);
        BOOST_TEST(id == storage_id);
    }

    storage_id = 0;
    storage_ptr = 0;
    p = pointer_traits::rr::get_global_pointer(storage_ptr, 0, storage_id);

    {
        auto [id, ptr] = pointer_traits::rr::get_storage_pointer(p);

        BOOST_TEST(ptr == storage_ptr);
        BOOST_TEST(id == storage_id);
    }
}

BOOST_AUTO_TEST_SUITE(a_ec_pointer_traits)

BOOST_AUTO_TEST_CASE(translates_storage_pointer_to_global_pointer) {
    const auto storage_id = 1;
    const auto storage_ptr = 21_KiB;

    auto group_ptr = pointer_traits::ec::get_global_pointer(
        storage_ptr, 0, storage_id, 10_KiB, 20_KiB);

    BOOST_TEST(group_ptr == 51_KiB);
}

BOOST_AUTO_TEST_CASE(translates_global_pointer_to_storage_pointer) {
    auto group_ptr = 31_KiB;

    auto [storage_id, storage_ptr] =
        pointer_traits::ec::get_storage_pointer(group_ptr, 10_KiB, 20_KiB);

    BOOST_TEST(storage_id == 1);
    BOOST_TEST(storage_ptr == 11_KiB);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
