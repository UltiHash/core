#define BOOST_TEST_MODULE "payg_handler tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/license/payg/payg.h>

using namespace uh::cluster;

BOOST_AUTO_TEST_SUITE(a_payg_license)

BOOST_AUTO_TEST_CASE(throws_for_invalid_json_string) {
    auto json_str = R"({"customer_id"? "big corp xy"})";

    BOOST_CHECK_THROW(payg_handler sut{json_str}, nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_CASE(throws_for_invalid_signature) {
    auto json_str = R"({
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

    BOOST_CHECK_THROW(payg_handler sut(json_str), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_for_no_signature) {
    auto json_str = R"({
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

    BOOST_CHECK_THROW(payg_handler sut(json_str), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(can_skip_validation) {
    auto json_str = R"({
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

    BOOST_CHECK_NO_THROW(
        payg_handler sut(json_str, payg_handler::verify::SKIP_VERIFY));
}

BOOST_AUTO_TEST_CASE(throws_for_missing_field) {
    auto json_str = R"({
    "customer_id": "big corp xy",
    "license_type": "freemium",
    "storage_cap": 10240,
    "ec": {
        "enabled": true,
        "max_group_size": 10
    },
})";

    BOOST_CHECK_THROW(
        payg_handler sut(json_str, payg_handler::verify::SKIP_VERIFY),
        nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_SUITE_END()

/*
 * Below, we are testing the payg_handler class with the correct JSON string.
 */
class fixture {

public:
    fixture()
        : sut{json_literal} {}

    static constexpr const char* json_literal = R"({
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
        "yg2DNf6iej5np/rQuM4mkp1xzByxxV6vHmHjrbimLyNndL+biWhajraNcp88mXB6iNy/EQ5Izx8H6Q7mggpxBg=="
    })";
    payg_handler sut;
};

BOOST_FIXTURE_TEST_SUITE(a_initialized_payg_license, fixture)

BOOST_AUTO_TEST_CASE(parses_license_to_payg) {
    auto license = sut.get();

    BOOST_CHECK_EQUAL(license.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(license.license_type, payg_license::type::FREEMIUM);
    BOOST_CHECK_EQUAL(license.storage_cap, 10240);
    BOOST_CHECK_EQUAL(license.ec.enabled, true);
    BOOST_CHECK_EQUAL(license.ec.max_group_size, 10);
    BOOST_CHECK_EQUAL(license.replication.enabled, true);
    BOOST_CHECK_EQUAL(license.replication.max_replicas, 3);
}

BOOST_AUTO_TEST_SUITE_END()
