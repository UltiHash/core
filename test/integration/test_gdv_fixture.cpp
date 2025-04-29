#define BOOST_TEST_MODULE "global_data_view_fixture tests"

#include <boost/test/unit_test.hpp>

#include <util/gdv_fixture.h>

namespace uh::cluster {

BOOST_FIXTURE_TEST_CASE(test_fixture, global_data_view_fixture) {
    // Do nothing. I'd like to test it's constructor and destuctor only.
}

} // namespace uh::cluster
