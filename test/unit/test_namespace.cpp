#define BOOST_TEST_MODULE "path tests"

#include <boost/test/unit_test.hpp>

#include <common/etcd/namespace.h>

namespace uh::cluster::ns {

BOOST_AUTO_TEST_SUITE(a_storage_group_namespace)

BOOST_AUTO_TEST_CASE(supports_root) {
    BOOST_TEST(std::string(root.storage_group) == "/uh/storage_group");
}

BOOST_AUTO_TEST_CASE(supports_group_configs) {
    BOOST_TEST(std::string(root.storage_group.group_configs) ==
               "/uh/storage_group/group_configs");
}

BOOST_AUTO_TEST_CASE(supports_group_config) {
    BOOST_TEST(std::string(root.storage_group.group_configs[2]) ==
               "/uh/storage_group/group_configs/2");
}

BOOST_AUTO_TEST_CASE(supports_storage_assignments) {
    BOOST_TEST(std::string(root.storage_group.storage_assignments[2]) ==
               "/uh/storage_group/storage_assignments/2");
}

// BOOST_AUTO_TEST_CASE(supports_storage_group_configs) {
//     BOOST_TEST(std::string(root.storage_group.group_configs[2]) ==
//                "/uh/storage_group/group_configs/2");
// }
//
// BOOST_AUTO_TEST_CASE(supports_internals) {
//
//     BOOST_TEST(std::string(root.storage_group.internals[2].storage_states[3])
//     ==
//                "/uh/storage_group/internals/2/storage_states/3");
//     BOOST_TEST(std::string(root.storage_group.internals[2].group_initialized)
//     ==
//                "/uh/storage_group/internals/2/group_initialized");
// }
//
BOOST_AUTO_TEST_CASE(supports_externals) {
    BOOST_TEST(
        std::string(root.storage_group.externals[2].storage_hostports[3]) ==
        "/uh/storage_group/externals/2/storage_hostports/3");
    BOOST_TEST(std::string(root.storage_group.externals[2].group_state) ==
               "/uh/storage_group/externals/2/group_state");
}
//
// BOOST_AUTO_TEST_CASE(supports_group_configs_and_storage_assignments) {
//     BOOST_TEST(std::string(root.storage_group.group_configs[2]) ==
//                "/uh/storage_group/group_configs/2");
//     BOOST_TEST(std::string(root.storage_group.storage_assignments[3]) ==
//                "/uh/storage_group/storage_assignments/3");
// }

BOOST_AUTO_TEST_SUITE_END()
} // namespace uh::cluster::ns
