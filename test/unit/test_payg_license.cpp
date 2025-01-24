#define BOOST_TEST_MODULE "license tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/license/payg.h>
#include <common/utils/strings.h>

using namespace uh::cluster;

BOOST_AUTO_TEST_CASE(test_check_payg_license) {
    std::string json_str = R"({
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
    "signature": "Bplb7lZQIK+mIXyPZKRNRIjara5EqxrCz8M5FDlPfqDrlMppL43axS7Ccd9TyuL4v03zHsFzPOyW7k+L+uouBw=="
})";

    auto license = check_payg_license(json_str, true);

    BOOST_CHECK_EQUAL(license.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(license.license_type, payg::type::FREEMIUM);
    BOOST_CHECK_EQUAL(license.storage_cap, 10240);
    BOOST_CHECK_EQUAL(license.ec.enabled, true);
    BOOST_CHECK_EQUAL(license.ec.max_group_size, 10);
    BOOST_CHECK_EQUAL(license.replication.enabled, true);
    BOOST_CHECK_EQUAL(license.replication.max_replicas, 3);
}
