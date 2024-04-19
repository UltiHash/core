#define BOOST_TEST_MODULE "pointer_traits tests"

#include <boost/test/unit_test.hpp>
#include <common/utils/pointer_traits.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(random_test) {
    int failures = 0;
    for (int i = 0; i < 100; ++i) {
        const auto service_id = rand() % std::numeric_limits<uint32_t>::max();
        const auto data_store_id = rand() % std::numeric_limits<uint32_t>::max();
        const auto pointer = rand() % std::numeric_limits<uint64_t>::max();
        auto p = pointer_traits::get_global_pointer(pointer, service_id, data_store_id);
        if (pointer_traits::get_data_store_id(p) != data_store_id) {
            failures++;
        }
        if (pointer_traits::get_pointer(p) != pointer) {
            failures++;
        }
        if (pointer_traits::get_service_id(p) != service_id) {
            failures++;
        }
    }
    BOOST_TEST(failures == 0);

}

BOOST_AUTO_TEST_CASE(edge_case_test) {
    auto service_id = std::numeric_limits<uint32_t>::max();
    auto data_store_id = std::numeric_limits<uint32_t>::max();
    auto pointer = std::numeric_limits<uint64_t>::max();
    auto p = pointer_traits::get_global_pointer(pointer, service_id, data_store_id);

    BOOST_TEST(pointer_traits::get_data_store_id(p) == data_store_id);
    BOOST_TEST(pointer_traits::get_pointer(p) == pointer);
    BOOST_TEST(pointer_traits::get_service_id(p) == service_id);

    service_id = 0;
    data_store_id = 0;
    pointer = 0;
    p = pointer_traits::get_global_pointer(pointer, service_id, data_store_id);

    BOOST_TEST(pointer_traits::get_data_store_id(p) == data_store_id);
    BOOST_TEST(pointer_traits::get_pointer(p) == pointer);
    BOOST_TEST(pointer_traits::get_service_id(p) == service_id);
}

} // namespace uh::cluster
