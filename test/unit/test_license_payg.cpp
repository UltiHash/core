#define BOOST_TEST_MODULE "payg_license tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/license/payg/payg.h>

using namespace uh::cluster;

BOOST_AUTO_TEST_SUITE(a_payg_license)

BOOST_AUTO_TEST_CASE(throws_for_invalid_json_string) {
    static constexpr const char* json_literal =
        R"({"customer_id"? "big corp xy"})";

    BOOST_CHECK_THROW(payg_license::create_from_json(json_literal),
                      nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_CASE(throws_for_invalid_signature) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap": 10240,
        "ec": {
            "enabled": true,
            "max_group_size": 10
        },
        "replication": {
          "enabled": true,
          "max_replicas": 3
        },
        "signature":
            "123=="
    })";

    BOOST_CHECK_THROW(payg_license::create_from_json(json_literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_for_no_signature) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap": 10240,
        "ec": {
            "enabled": true,
            "max_group_size": 10
        },
        "replication": {
          "enabled": true,
          "max_replicas": 3
        }
    })";

    BOOST_CHECK_THROW(payg_license::create_from_json(json_literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(can_skip_validation) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "UltiHash-Test",
        "license_type": "freemium",
        "storage_cap": 1048576,
        "ec": {
            "enabled": true,
            "max_group_size": 10
        },
        "replication": {
          "enabled": true,
          "max_replicas": 3
        }
    })";

    BOOST_CHECK_NO_THROW(payg_license::create_from_json(
        json_literal, payg_license::verify::SKIP_VERIFY));
}

BOOST_AUTO_TEST_CASE(throws_for_missing_field) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap": 10240,
        "ec": {
            "enabled": true,
            "max_group_size": 10
        },
    })";

    BOOST_CHECK_THROW(payg_license::create_from_json(
                          json_literal, payg_license::verify::SKIP_VERIFY),
                      nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_SUITE_END()

/*******************************************************************************
 * Below, we are testing the payg_license class with the correct JSON string.
 */
class fixture {

public:
    fixture()
        : sut{payg_license::create_from_json(json_literal)} {}

    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap": 10240,
        "ec": {
            "enabled": true,
            "max_group_size": 10
        },
        "replication": {
            "enabled": true,
            "max_replicas": 3
        },
        "signature": 
        "wU8LnUXWavhYSK/xejVheVYoORaHPYGkYOeK3Y5HSwT8Nzo00cDGhwHoM2r0vRAgNMec5xWzJaIgUI2WhvDYBg=="
    })";
    static constexpr const char* json_compact_literal =
        R"({"version":"v1","customer_id":"big corp xy","license_type":"freemium","storage_cap":10240,"ec":{"enabled":true,"max_group_size":10},"replication":{"enabled":true,"max_replicas":3}})";

    payg_license sut;
};

BOOST_FIXTURE_TEST_SUITE(a_initialized_payg_license, fixture)

BOOST_AUTO_TEST_CASE(parses_license_to_payg) {

    BOOST_CHECK_EQUAL(sut.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(sut.license_type, payg_license::type::FREEMIUM);
    BOOST_CHECK_EQUAL(sut.storage_cap, 10240);
    BOOST_CHECK_EQUAL(sut.ec.enabled, true);
    BOOST_CHECK_EQUAL(sut.ec.max_group_size, 10);
    BOOST_CHECK_EQUAL(sut.replication.enabled, true);
    BOOST_CHECK_EQUAL(sut.replication.max_replicas, 3);
}

BOOST_AUTO_TEST_CASE(prints_out_compact_form_json_string) {
    auto compact_json_str = sut.to_json_string();

    BOOST_TEST(compact_json_str == json_compact_literal);
}

BOOST_AUTO_TEST_SUITE_END()
