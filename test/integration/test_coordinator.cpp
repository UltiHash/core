#define BOOST_TEST_MODULE "coordinator tests"

#include <boost/test/unit_test.hpp>

#include <coordinator/service.h>

#include <stdexcept>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

const storage::group_config& config_rr = {
    .id = 0,
    .type = storage::group_config::type_t::ROUND_ROBIN,
    .storages = 3,
};

const storage::group_config& config_ec_1k = {
    .id = 0,
    .type = storage::group_config::type_t::ERASURE_CODING,
    .storages = 3,
    .data_shards = 2,
    .parity_shards = 1,
    .stripe_size_kib = 1024,
};

const storage::group_config& config_ec_2k = {
    .id = 0,
    .type = storage::group_config::type_t::ERASURE_CODING,
    .storages = 3,
    .data_shards = 2,
    .parity_shards = 1,
    .stripe_size_kib = 2024,
};

BOOST_AUTO_TEST_CASE(start_no_previous_config) {
    etcd_manager etcd;
    etcd.clear_all();
    std::this_thread::sleep_for(100ms);
    {
        auto retrieved = etcd.get(ns::root.storage_groups.group_configs[0]);
        BOOST_CHECK_EQUAL("", retrieved);
    }

    {
        storage::group_configs configs;
        configs.configs.push_back(config_rr);
        BOOST_CHECK_NO_THROW(
            coordinator::service::publish_configs(etcd, configs));
        auto retrieved =
            etcd.get(ns::root.storage_groups.group_configs[config_rr.id]);
        BOOST_CHECK_EQUAL(config_rr.to_string(), retrieved);
    }
}

BOOST_AUTO_TEST_CASE(start_matching_config) {
    etcd_manager etcd;
    etcd.clear_all();
    std::this_thread::sleep_for(100ms);
    {
        auto retrieved = etcd.get(ns::root.storage_groups.group_configs[0]);
        BOOST_CHECK_EQUAL("", retrieved);
    }

    {
        storage::group_configs configs;
        configs.configs.push_back(config_ec_1k);
        BOOST_CHECK_NO_THROW(
            coordinator::service::publish_configs(etcd, configs));
        auto retrieved =
            etcd.get(ns::root.storage_groups.group_configs[config_ec_1k.id]);
        BOOST_CHECK_EQUAL(config_ec_1k.to_string(), retrieved);
    }

    {
        storage::group_configs configs;
        configs.configs.push_back(config_ec_1k);
        BOOST_CHECK_NO_THROW(
            coordinator::service::publish_configs(etcd, configs));
        auto retrieved =
            etcd.get(ns::root.storage_groups.group_configs[config_ec_1k.id]);
        BOOST_CHECK_EQUAL(config_ec_1k.to_string(), retrieved);
    }
}

BOOST_AUTO_TEST_CASE(start_mismatching_config) {
    etcd_manager etcd;
    etcd.clear_all();
    std::this_thread::sleep_for(100ms);
    {
        auto retrieved = etcd.get(ns::root.storage_groups.group_configs[0]);
        BOOST_CHECK_EQUAL("", retrieved);
    }

    {
        storage::group_configs configs;
        configs.configs.push_back(config_ec_1k);
        BOOST_CHECK_NO_THROW(
            coordinator::service::publish_configs(etcd, configs));
        auto retrieved =
            etcd.get(ns::root.storage_groups.group_configs[config_ec_1k.id]);
        BOOST_CHECK_EQUAL(config_ec_1k.to_string(), retrieved);
    }

    {
        storage::group_configs configs;
        configs.configs.push_back(config_ec_2k);
        BOOST_CHECK_THROW(coordinator::service::publish_configs(etcd, configs),
                          std::runtime_error);
        auto retrieved =
            etcd.get(ns::root.storage_groups.group_configs[config_ec_2k.id]);
        BOOST_CHECK_EQUAL(config_ec_1k.to_string(), retrieved);
    }
}

} // namespace uh::cluster
