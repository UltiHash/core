#define BOOST_TEST_MODULE "storage group state tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <storage/group/state.h>

using namespace uh::cluster::storage_group;

BOOST_AUTO_TEST_SUITE(a_storage_group_config)

BOOST_AUTO_TEST_CASE(throws_when_theres_no_comma) {
    static constexpr const char* literal = "00102";

    BOOST_CHECK_THROW(state::create(literal), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_for_comma_on_wrong_position) {
    static constexpr const char* literal = "00,102";

    BOOST_CHECK_THROW(state::create(literal), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_when_group_state_is_out_of_range) {
    static constexpr const char* literal = "9,101";

    BOOST_CHECK_THROW(state::create(literal), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_when_storage_state_is_out_of_range) {
    static constexpr const char* literal = "0,103";

    BOOST_CHECK_THROW(state::create(literal), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(is_well_created) {
    static constexpr const char* literal = "0,102";

    auto state = state::create(literal);

    BOOST_TEST(static_cast<int>(state.group) == 0);
    BOOST_TEST(state.storages.size() == 3);
    BOOST_TEST(static_cast<int>(state.storages[0]) == 1);
    BOOST_TEST(static_cast<int>(state.storages[1]) == 0);
    BOOST_TEST(static_cast<int>(state.storages[2]) == 2);
}

BOOST_AUTO_TEST_CASE(to_string_is_idempotent) {
    static constexpr const char* literal = "0,102";
    auto state = state::create(literal);

    auto str = state.to_string();

    BOOST_TEST(str == literal);
}

BOOST_AUTO_TEST_SUITE_END()
