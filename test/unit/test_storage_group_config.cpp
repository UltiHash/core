#define BOOST_TEST_MODULE "storage group config tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <storage/group/config.h>

#include <nlohmann/json.hpp>

namespace uh::cluster::storage {

BOOST_AUTO_TEST_SUITE(a_storage_group_config)

BOOST_AUTO_TEST_CASE(throws_for_invalid_json_string) {
    static constexpr const char* json_literal =
        R"([{id:0,"type":"ERASURE_CODING","storages":3,"data_shards":2,"parity_shards":1,"stripe_size_kib":1024},{"id":1,"type":"ROUND_ROBIN","storages":2}])";

    BOOST_CHECK_THROW(group_configs::create(json_literal),
                      nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_CASE(throws_when_stripe_size_is_not_divisible_by_storages) {
    static constexpr const char* json_literal =
        R"([{"id":0,"type":"ERASURE_CODING","storages":4,"data_shards":3,"parity_shards":1,"stripe_size_kib":1024}])";

    BOOST_CHECK_THROW(group_configs::create(json_literal),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(
    throws_when_storages_count_is_not_equal_to_data_and_parity) {
    static constexpr const char* json_literal =
        R"([{"id":0,"type":"ERASURE_CODING","storages":4,"data_shards":2,"parity_shards":1,"stripe_size_kib":1024}])";

    BOOST_CHECK_THROW(group_configs::create(json_literal),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(throws_for_missing_parity_shards_for_ec_group) {
    static constexpr const char* json_literal =
        R"([{"id":0,"type":"ERASURE_CODING","storages":3,"data_shards":2,"stripe_size_kib":1024}])";

    BOOST_CHECK_THROW(group_configs::create(json_literal),
                      nlohmann::json::out_of_range);
}

BOOST_AUTO_TEST_CASE(throws_for_missing_storages_for_rr_group) {
    static constexpr const char* json_literal =
        R"([{"id":0,"type":"ROUND_ROBIN"}])";

    BOOST_CHECK_THROW(group_configs::create(json_literal),
                      nlohmann::json::out_of_range);
}

BOOST_AUTO_TEST_CASE(deserializes_rr_group) {
    static constexpr const char* json_literal =
        R"([{"id":0,"type":"ROUND_ROBIN","storages":2}])";

    BOOST_TEST(group_configs::create(json_literal).to_string() == json_literal);
}

BOOST_AUTO_TEST_CASE(deserializes_ec_group) {
    static constexpr const char* json_literal =
        R"([{"id":0,"type":"ERASURE_CODING","storages":3,"data_shards":2,"parity_shards":1,"stripe_size_kib":1024}])";

    BOOST_TEST(group_configs::create(json_literal).to_string() == json_literal);
}

BOOST_AUTO_TEST_SUITE_END()

/*******************************************************************************
 * Below, we are testing the group_config class with the correct JSON
 * string.
 */
class fixture {

public:
    fixture()
        : sut{group_configs::create(test_storage_group_config_string)} {}

    group_configs sut;
};

BOOST_FIXTURE_TEST_SUITE(a_initialized_license, fixture)

BOOST_AUTO_TEST_CASE(parses_json_string_to_license) {
    BOOST_TEST(sut.configs[0].type == group_config::type_t::ERASURE_CODING);
    BOOST_TEST(sut.configs[0].storages == 3);
    BOOST_TEST(sut.configs[0].data_shards == 2);
    BOOST_TEST(sut.configs[0].parity_shards == 1);
    BOOST_TEST(sut.configs[0].stripe_size_kib == 1024);
    BOOST_TEST(sut.configs[1].type == group_config::type_t::ROUND_ROBIN);
    BOOST_TEST(sut.configs[1].storages == 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
