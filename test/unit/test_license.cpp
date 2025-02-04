#define BOOST_TEST_MODULE "license tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/license/license.h>

using namespace uh::cluster;

BOOST_AUTO_TEST_SUITE(a_license)

BOOST_AUTO_TEST_CASE(throws_for_invalid_json_string) {
    static constexpr const char* json_literal =
        R"({"customer_id"? "big corp xy"})";

    BOOST_CHECK_THROW(license::create(json_literal),
                      nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_CASE(throws_for_invalid_signature) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap_gib": 10240,
        "signature":
            "123=="
    })";

    BOOST_CHECK_THROW(license::create(json_literal), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_for_no_signature) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap_gib": 10240
    })";

    BOOST_CHECK_THROW(license::create(json_literal), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(can_skip_validation) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "UltiHash-Test",
        "license_type": "freemium",
        "storage_cap_gib": 1048576
    })";

    BOOST_CHECK_NO_THROW(
        license::create(json_literal, license::verify::SKIP_VERIFY));
}

BOOST_AUTO_TEST_CASE(throws_for_missing_field) {
    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
    })";

    BOOST_CHECK_THROW(
        license::create(json_literal, license::verify::SKIP_VERIFY),
        nlohmann::json::parse_error);
}

BOOST_AUTO_TEST_SUITE_END()

/*******************************************************************************
 * Below, we are testing the license class with the correct JSON string.
 */
class fixture {

public:
    fixture()
        : sut{license::create(test_license_string)} {}

    license sut;
};

BOOST_FIXTURE_TEST_SUITE(a_initialized_license, fixture)

BOOST_AUTO_TEST_CASE(parses_json_string_to_license) {

    BOOST_CHECK_EQUAL(sut.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(sut.license_type, license::type::FREEMIUM);
    BOOST_CHECK_EQUAL(sut.storage_cap_gib, 10240);
}

BOOST_AUTO_TEST_CASE(prints_out_compact_form_json_string) {
    auto compact_json_str = sut.to_string();

    BOOST_TEST(compact_json_str == test_license_string);
}

BOOST_AUTO_TEST_SUITE_END()
