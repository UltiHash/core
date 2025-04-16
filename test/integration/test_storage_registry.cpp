#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_MODULE "storage_registry tests"

#include <common/etcd/namespace.h>
#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/utils.h>
#include <util/temp_directory.h>

#include <boost/asio/ip/host_name.hpp>
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(basic_register_retrieve_update_retrieve_deregister) {

    const auto service_id = 23;
    const auto group_id = 42;
    const auto port_address = 9200;

    auto etcd = etcd_manager();

    {
        // check if the keys already exist or not
        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, service_id) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, service_id) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));

        BOOST_TEST(host.empty());
        BOOST_TEST(port.empty());

        BOOST_TEST(!etcd.has(get_announced_path(STORAGE_SERVICE, service_id)));
    }

    {
        temp_directory tmp_dir;
        storage_registry registering_registry(service_id, group_id, etcd,
                                              tmp_dir.path());
        registering_registry.register_service({.port = port_address});

        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, service_id) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        BOOST_TEST(host == boost::asio::ip::host_name());

        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, service_id) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));
        BOOST_TEST(std::stoul(port) == port_address);

        const auto announced_etcd_path =
            get_announced_path(STORAGE_SERVICE, service_id);
        BOOST_TEST(get_announced_id(announced_etcd_path) == service_id);
        BOOST_TEST(etcd.has(announced_etcd_path));

        const auto storage_group_map =
            etcd.get(get_storage_to_storage_group_map_path(service_id));
        BOOST_TEST(std::stoul(storage_group_map) == group_id);

        const std::string storage_state_path =
            get_storage_groups_assigned_storages_path(group_id, service_id);
        {
            const auto storage_state_number =
                std::stoul(etcd.get(storage_state_path));
            const auto storage_state =
                magic_enum::enum_cast<storage_registry::storage_state>(
                    storage_state_number);
            BOOST_TEST(storage_state.has_value());
            BOOST_TEST((storage_state.value() ==
                        storage_registry::storage_state::NEW));
        }
        {
            registering_registry.update_service_state(
                storage_registry::storage_state::ASSIGNED);
            sleep(1);
            const auto storage_state_number =
                std::stoul(etcd.get(storage_state_path));
            const auto storage_state =
                magic_enum::enum_cast<storage_registry::storage_state>(
                    storage_state_number);
            BOOST_TEST(storage_state.has_value());
            BOOST_TEST((storage_state.value() ==
                        storage_registry::storage_state::ASSIGNED));
        }
    }

    {
        // check for de-registry
        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, service_id) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, service_id) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));

        BOOST_TEST(host.empty());
        BOOST_TEST(port.empty());

        BOOST_TEST(!etcd.has(get_announced_path(STORAGE_SERVICE, service_id)));
    }
}

} // end namespace uh::cluster
