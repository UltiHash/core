#define BOOST_TEST_MODULE "storage group config tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <storage/group/config.h>

#include <nlohmann/json.hpp>

using namespace uh::cluster;

BOOST_AUTO_TEST_SUITE(a_storage_group_config)

BOOST_AUTO_TEST_CASE(throws_for_invalid_json_string) {
    static constexpr const char* json_literal =
        R"([{"data_shards:3,"parity_shards":1,"members":[0,1,2,3]},{"data_shards":2,"parity_shards":0,"members":[4,5]}])";

    BOOST_CHECK_THROW(storage::group::config::create_multiple(json_literal),
                      nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_CASE(throws_for_missing_field) {
    static constexpr const char* json_literal =
        R"([{"data_shards":3,"parity_shards":1},{"data_shards":2,"parity_shards":0}])";

    BOOST_CHECK_THROW(storage::group::config::create_multiple(json_literal),
                      nlohmann::json::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()

/*******************************************************************************
 * Below, we are testing the storage::group::config class with the correct JSON
 * string.
 */
class fixture {

public:
    fixture()
        : sut{storage::group::config::create_multiple(
              test_storage_group_config_string)} {}

    std::vector<storage::group::config> sut;
};

BOOST_FIXTURE_TEST_SUITE(a_initialized_license, fixture)

BOOST_AUTO_TEST_CASE(parses_json_string_to_license) {
    BOOST_CHECK_EQUAL(sut[0].data_shards, 3);
    BOOST_CHECK_EQUAL(sut[0].parity_shards, 1);
    BOOST_CHECK_EQUAL(sut[0].members.size(), 4);
    BOOST_CHECK_EQUAL(sut[0].members[0], 0);
    BOOST_CHECK_EQUAL(sut[0].members[1], 1);
    BOOST_CHECK_EQUAL(sut[0].members[2], 2);
    BOOST_CHECK_EQUAL(sut[0].members[3], 3);
    BOOST_CHECK_EQUAL(sut[1].data_shards, 2);
    BOOST_CHECK_EQUAL(sut[1].parity_shards, 0);
    BOOST_CHECK_EQUAL(sut[1].members.size(), 2);
    BOOST_CHECK_EQUAL(sut[1].members[0], 4);
    BOOST_CHECK_EQUAL(sut[1].members[1], 5);
    // BOOST_CHECK_EQUAL(sut[0].members, {0, 1, 2, 3});
    // BOOST_CHECK_EQUAL(sut.license_type, license::type::FREEMIUM);
    // BOOST_CHECK_EQUAL(sut.storage_cap_gib, 10240);
}

BOOST_AUTO_TEST_SUITE_END()
