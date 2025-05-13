#define BOOST_TEST_MODULE "path tests"

#include <boost/test/unit_test.hpp>

#include <common/etcd/namespace.h>

namespace uh::cluster::ns {

BOOST_AUTO_TEST_CASE(a_root_supports_group_namespace) {
    BOOST_TEST(std::string(root.storage_groups) == "/uh/storage_groups");
    BOOST_TEST(std::string(root.storage_groups.group_configs) ==
               "/uh/storage_groups/group_configs");
    BOOST_TEST(std::string(root.storage_groups.group_configs[2]) ==
               "/uh/storage_groups/group_configs/2");
    BOOST_TEST(std::string(root.storage_groups[2].storage_states[3]) ==
               "/uh/storage_groups/2/storage_states/3");
    BOOST_TEST(std::string(root.storage_groups[2].group_initialized) ==
               "/uh/storage_groups/2/group_initialized");
    BOOST_TEST(std::string(root.storage_groups[2].storage_hostports[3]) ==
               "/uh/storage_groups/2/storage_hostports/3");
    BOOST_TEST(std::string(root.storage_groups[2].group_state) ==
               "/uh/storage_groups/2/group_state");
}

} // namespace uh::cluster::ns
