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
        const auto service_id = rand() % std::numeric_limits<uint32_t>::max();
        const auto pointer = rand() % std::numeric_limits<uint64_t>::max();
        auto p =
            pointer_traits::rr::get_global_pointer(pointer, 0, service_id, 0);
        if (pointer_traits::get_pointer(p) != pointer) {
            failures++;
        }
        if (pointer_traits::rr::get_storage_id(p) != service_id) {
            failures++;
        }
    }
    BOOST_TEST(failures == 0);
}

BOOST_AUTO_TEST_CASE(edge_case_test) {
    auto service_id = std::numeric_limits<uint32_t>::max();
    auto data_store_id = std::numeric_limits<uint32_t>::max();
    auto pointer = std::numeric_limits<uint64_t>::max();
    auto p = pointer_traits::rr::get_global_pointer(pointer, 0, service_id,
                                                    data_store_id);

    BOOST_TEST(pointer_traits::get_pointer(p) == pointer);
    BOOST_TEST(pointer_traits::rr::get_storage_id(p) == service_id);

    service_id = 0;
    data_store_id = 0;
    pointer = 0;
    p = pointer_traits::rr::get_global_pointer(pointer, 0, service_id,
                                               data_store_id);

    BOOST_TEST(pointer_traits::get_pointer(p) == pointer);
    BOOST_TEST(pointer_traits::rr::get_storage_id(p) == service_id);
}

BOOST_AUTO_TEST_SUITE(a_pointer_traits)

BOOST_AUTO_TEST_CASE(translates_storage_pointer_to_group_pointer) {
    const auto storage_id = 1;
    const auto storage_ptr = 21_KiB;

    auto group_ptr = pointer_traits::ec::group_pointer(storage_id, storage_ptr,
                                                       10_KiB, 20_KiB);

    BOOST_TEST(group_ptr == 51_KiB);
}

BOOST_AUTO_TEST_CASE(translates_group_pointer_to_storage_pointer) {
    auto group_ptr = 31_KiB;

    auto [storage_id, storage_ptr] =
        pointer_traits::ec::storage_pointer(group_ptr, 10_KiB, 20_KiB);

    BOOST_TEST(storage_id == 1);
    BOOST_TEST(storage_ptr == 11_KiB);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
