#define BOOST_TEST_MODULE "path tests"

#include <boost/test/unit_test.hpp>

#include <common/etcd/namespace.h>

namespace uh::cluster::ns {

BOOST_AUTO_TEST_CASE(supports_slash_operator) {

    BOOST_TEST(std::string(root.storage_group.internals[2].storage_states[3]) ==
               "/uh/storage_groups/internals/2/storage_states/3");
    BOOST_TEST(std::string(root.storage_group.internals[2].group_initialized) ==
               "/uh/storage_groups/internals/2/group_initialized");
}

BOOST_AUTO_TEST_CASE(supports_external_path) {
    BOOST_TEST(std::string(root.storage_group.externals[2].hostports[3]) ==
               "/uh/storage_groups/externals/2/hostports/3");
    BOOST_TEST(std::string(root.storage_group.externals[2].group_state) ==
               "/uh/storage_groups/externals/2/group_state");
}

} // namespace uh::cluster::ns
